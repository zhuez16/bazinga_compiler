//
// Created by 顾超 on 2021/7/22.
//

#include "pass/combining.h"

int sp_add(int l, int r) { return l + r; }
int sp_sub(int l, int r) { return l - r; }
int sp_mul(int l, int r) { return l * r; }
int sp_div(int l, int r) { return (r) ? l / r : 0; }
int sp_mod(int l, int r) { return (r) ? l % r : 0; }

void ConstFoldingDCEliminating::run() {
    //Test 1: 只做常量折叠

    for (auto function: m_->get_functions()){
        std::vector<Instruction *> inst_tbd;
        // Step 1 遍历所有指令，有返回值的全部加入map中
        // 将所有值压入worklist中
        for(auto bb: function->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                if (!inst->is_void()) {
                    if (inst->isStaticCalculable()) {
                        int constResult = inst->calculate();
                        setLattice(inst, ValueLattice(constResult));
                        inst->replace_all_use_with(ConstantInt::get(constResult, inst->get_module()));
                    } else {
                        setLattice(inst, ValueLattice());
                        pushWorkList(inst);
                    }
                }
            }
        }
        // Step 2 不动点迭代直至worklist为空，暂时只做Binary测试
        Instruction *proceed_value;
        while ((proceed_value = popWorkList()) != nullptr) {
            if (proceed_value->isBinary()) {
                auto binaryInst = dynamic_cast<BinaryInst *>(proceed_value);
                auto lhs_l = getLatticeByValue(binaryInst->get_operand(0));
                auto rhs_l = getLatticeByValue(binaryInst->get_operand(1));
                ValueLattice result_l;
                switch (binaryInst->get_instr_type()) {
                    case Instruction::add:
                        result_l = ValueLattice::operand(lhs_l, rhs_l, &sp_add, false);
                        break;
                    case Instruction::sub:
                        result_l = ValueLattice::operand(lhs_l, rhs_l, &sp_sub, false);
                        break;
                    case Instruction::mul:
                        result_l = ValueLattice::operand(lhs_l, rhs_l, &sp_mul, true);
                        break;
                    case Instruction::sdiv:
                        result_l = ValueLattice::operand(lhs_l, rhs_l, &sp_div, false);
                        break;
                    case Instruction::mod:
                        result_l = ValueLattice::operand(lhs_l, rhs_l, &sp_mod, false);
                        break;
                    default:
                        assert(0 && "Unsupported instr type");
                }
                auto orig_l = getLatticeByValue(binaryInst);
                if (orig_l != result_l) {
                    // Update usage
                    for(auto user: binaryInst->get_use_list()) {
                        auto inst_tbu = dynamic_cast<Instruction *>(user.val_);
                        if (inst_tbu && !inst_tbu->is_void()) {
                            pushWorkList(inst_tbu);
                        }
                    }
                    setLattice(binaryInst, result_l);
                }
            }
        }


        // Step 3 对所有结点应用变换。若是某结点Lattice标记为Constant则将其替换为CONST
        for(auto bb: function->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                if (!inst->is_void()) {
                    ValueLattice ll = getLatticeByValue(inst);
                    if (ll.isConstantOrZeroType()) {
                        inst->replace_all_use_with(ConstantInt::get(ll.getValue(), inst->get_module()));
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
