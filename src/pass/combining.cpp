//
// Created by 顾超 on 2021/7/22.
//

#include "pass/combining.h"

int sp_add(int l, int r) { return l + r; }
int sp_sub(int l, int r) { return l - r; }
int sp_mul(int l, int r) { return l * r; }
int sp_div(int l, int r) { return (r) ? l / r : 0; }
int sp_mod(int l, int r) { return (r) ? l % r : 0; }
int sp_eq(int l, int r) { return l == r; }
int sp_ne(int l, int r) { return l != r; }
int sp_gt(int l, int r) { return l > r; }
int sp_ge(int l, int r) { return l >= r; }
int sp_lt(int l, int r) { return l < r; }
int sp_le(int l, int r) { return l <= r; }

std::map<Instruction::OpID, int (*)(int, int)> OPId2Inst = {
        {Instruction::OpID::add, &sp_add},
        {Instruction::OpID::sub, &sp_sub},
        {Instruction::OpID::mul, &sp_mul},
        {Instruction::OpID::sdiv, &sp_div},
        {Instruction::OpID::mod, &sp_mod}
};

std::map<CmpInst::CmpOp, int (*)(int, int)> CmpID2Inst = {
        {CmpInst::CmpOp::LT, &sp_lt},
        {CmpInst::CmpOp::LE, &sp_le},
        {CmpInst::CmpOp::EQ, &sp_eq},
        {CmpInst::CmpOp::NE, &sp_ne},
        {CmpInst::CmpOp::GT, &sp_gt},
        {CmpInst::CmpOp::GE, &sp_ge},
};

void ConstFoldingDCEliminating::run() {
    //Test 1: 只做常量折叠

    for (auto function: m_->get_functions()){
        if (function->is_declaration()) continue;
        std::vector<Instruction *> inst_tbd;
        // Step 1 遍历所有指令，有返回值的全部加入map中
        // 将所有值压入worklist中
        for(auto bb: function->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                if (!inst->is_void()) {
                    if (inst->isStaticCalculable()) {
                        int constResult = inst->calculate();
                        setLattice(inst, Lattice(ValueLattice(constResult)));
                        if (inst->is_cmp()) {
                            inst->replace_all_use_with(ConstantInt::get((bool)constResult, inst->get_module()));
                        } else {
                            inst->replace_all_use_with(ConstantInt::get(constResult, inst->get_module()));
                        }
                    } else {
                        setLattice(inst, Lattice(ValueLattice()));
                        pushWorkList(inst);
                    }
                } else {
                    setLattice(inst, Lattice());
                }
            }
        }
        // 起步设置，将Initial块的Inst设为Reachable
        Instruction *entry = function->get_entry_block()->get_instructions().front();
        setLattice(entry, Lattice(getLatticeByValue(entry).val, InstLattice(InstLattice::ReachableTy)));
        // 填充指令关系序列，使每个指令可以获得与其到达性有关的属性(前继)
        for(auto bb: function->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                // Branch特殊处理
                if (inst->is_br()) {
                    auto br = dynamic_cast<BranchInst *>(inst);
                    if (br->is_cond_br()) {
                        addLink(br->getTrueBB()->get_instructions().front(), ConditionLink(br, br->get_condition(), ConditionLink::TrueCond));
                        addLink(br->getFalseBB()->get_instructions().front(), ConditionLink(br, br->get_condition(), ConditionLink::FalseCond));
                    } else {
                        addLink(br->getTrueBB()->get_instructions().front(), ConditionLink(br));
                    }
                } else {
                    addLink(inst->getSuccInst(), ConditionLink(inst));
                }
            }
        }
        /** Step 2 不动点迭代直至worklist为空
         */
        Instruction *proceed_value;
        while ((proceed_value = popWorkList()) != nullptr) {
            Lattice ll = getLatticeByValue(proceed_value);
            // Sub Step 1: 可达性计算
            InstLattice l;
            ValueLattice v = ll.val;
            // 获取所有前继
            // 计算 S1 = Sa + Sb * (pred <= T/F)
            auto links = getLink(proceed_value);
            if (links.empty()) {
                // 为空即为Entry指令或不可达块的首指令
                l = ll.inst;
            } else {
                // 否则进行标准计算
                for (ConditionLink link : links) {
                    if (link.type == ConditionLink::NoCond) {
                        l = l + getLatticeByValue(link.from).inst;
                    } else if (link.type == ConditionLink::TrueCond) {
                        l = l + (getLatticeByValue(link.from).inst *
                                 InstLattice::testing(getLatticeByValue(link.condition).val, true));
                    } else {
                        l = l + (getLatticeByValue(link.from).inst *
                                 InstLattice::testing(getLatticeByValue(link.condition).val, false));
                    }
                }
            }
            // Sub Step 2：Value计算
            switch (proceed_value->get_instr_type()) {
                /**
                 * Binary Op: a op b -> int32
                 * $ S_{i} = S_{i-1}$
                 * $ x_i = [S_{i} => f_{x_0, x_1,...}]$
                 */
                case Instruction::add:
                case Instruction::sub:
                case Instruction::mul:
                case Instruction::sdiv:
                case Instruction::mod: {
                    auto bin = dynamic_cast<BinaryInst *>(proceed_value);
                    auto l_vl = getLatticeByValue(bin->get_operand(0)).val;
                    auto r_vl = getLatticeByValue(bin->get_operand(1)).val;
                    // $f_{op}(x1, x2)$
                    v = ValueLattice::operand(l_vl, r_vl, OPId2Inst[bin->get_instr_type()], bin->get_instr_type() == Instruction::mul);
                    // S => f
                    v = InstLattice::unreached(l, v);
                    break;
                }
                case Instruction::cmp: {
                    auto cmp = dynamic_cast<CmpInst *>(proceed_value);
                    auto l_vl = getLatticeByValue(cmp->get_operand(0)).val;
                    auto r_vl = getLatticeByValue(cmp->get_operand(1)).val;
                    // $f_{op}(x1, x2)$
                    v = ValueLattice::operand(l_vl, r_vl, CmpID2Inst[cmp->get_cmp_op()], false);
                    v = InstLattice::unreached(l, v);
                    break;
                }
                case Instruction::phi: {
                    auto phi = dynamic_cast<PhiInst *>(proceed_value);
                    ValueLattice vv = getLatticeByValue(phi->get_operand(0)).val;
                    for (int i = 1; i < phi->get_num_operand() / 2; ++i) {
                        vv = ValueLattice::meet(vv, getLatticeByValue(phi->get_operand(i * 2)).val);
                    }
                    v = InstLattice::unreached(l, vv);
                    break;
                }
                case Instruction::call: {
                    auto call = dynamic_cast<CallInst *>(proceed_value);
                    if (!dynamic_cast<Function *>(call->get_operand(0))->get_function_type()->get_return_type()->is_void_type()) {
                        v = ValueLattice(ValueLattice::DownTy);
                    } else {
                        v = ValueLattice(ValueLattice::EmptyTy);
                    }
                    break;
                }
                case Instruction::zext: {
                    v = getLatticeByValue(proceed_value->get_operand(0)).val;
                    v = InstLattice::unreached(l, v);
                    break;
                }
                default:
                    // A instruction with no return value has a Empty value lattice
                    break;
            }
            // 与原本的进行对比
            Lattice new_la(v, l);
            if (new_la != ll) {
                // 更新map并将所有引用插入列表中
                setLattice(proceed_value, new_la);
                for (auto user: proceed_value->get_use_list()) {
                    pushWorkList(dynamic_cast<Instruction *>(user.val_));
                }
            }
        }


        // Step 3 对所有结点应用变换。若是某结点Lattice标记为Constant则将其替换为CONST，若某结点被标记为Unreachable则删除该结点
        for(auto bb: function->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                if (!inst->is_void()) {
                    Lattice lattice = getLatticeByValue(inst);
                    if (lattice.val.isConstantOrZeroType()) {
                        inst->replace_all_use_with(ConstantInt::get(lattice.val.getValue(), inst->get_module()));
                        inst_tbd.push_back(inst);
                    }
                    else if (lattice.inst.isUnreachable()) {
                        inst_tbd.push_back(inst);
                    }
                }
            }
        }

        // Step 4 删除所有无用的指令
        for(Instruction * inst: inst_tbd) {
            inst->get_parent()->delete_instr(inst);
        }
    }
}
