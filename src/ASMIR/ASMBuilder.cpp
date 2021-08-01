//
// Created by 顾超 on 2021/7/30.
//

#include "ASMIR/ASMBuilder.h"

bool inverseOperandIfNeeded(Value *&lhs, Value *&rhs) {
    assert(!(dynamic_cast<ConstantInt *>(lhs) && dynamic_cast<ConstantInt *>(rhs)) && "ARM doesn't support cmp between Imme. Please run ConstantFolding first.");
    // 第一个操作数不能是立即数
    if (dynamic_cast<ConstantInt *>(lhs)) {
        std::swap(lhs, rhs);
        return false;
    } else {
        return true;
    }
}

void getDimensionByType(std::vector<int> &dim, Type *ty) {
    if (!ty->is_array_type()) return;
    auto arr_ty = dynamic_cast<ArrayType *>(ty);
    dim.push_back(arr_ty->get_num_of_elements());
    getDimensionByType(dim, arr_ty->get_array_element_type());
}

void ASMBuilder::build(Module *m) {
    _mapping.clear();
    // Add all global values
    for (auto gv: m->get_global_variable()) {
        setGlobalValue(gv, ASGlobalValue::create(gv->get_name(), gv->get_type(), gv->get_init()));
    }
    // Prepare all labels, so that br inst can find its destination
    for (auto f: m->get_functions()) {
        if (!f->is_declaration()) {
            // Step 1. Add all function to mapping
            auto asFunc = ASFunction::createFunction(f->get_name(), (int)f->get_num_of_args());
            setFunction(f, asFunc);
            // Step 2. Add function arguments to mapping
            // At this time we assume that we have infinity registers, so all params are passed through register
            // Don't consider opt here. Push r0 - r3 into stack if needed
            int i = 0;
            for (auto arg: f->get_args()) {
                asFunc->setArgumentMapping(i++, arg);
            }
            // Step 3. Add all basic blocks to mapping
            for (auto bb: f->get_basic_blocks()) {
                _mapping[bb] = ASBlock::createBlock(asFunc, bb->get_name());
            }
        } else {
            _mapping[f] = ASFunction::createFunction(f->get_name(), f->get_num_of_args());
        }
    }
    // Code gen
    for (auto f: m->get_functions()) {
        if (!f->is_declaration()) {
            for (auto bb: f->get_basic_blocks()) {
                setInsertBlock(bb);
                for (auto inst: bb->get_instructions()) {
                    switch (inst->get_instr_type()) {
                        case Instruction::ret: {
                            auto ret = dynamic_cast<ReturnInst *>(inst);
                            // Check if it is a void function
                            if (!ret->is_void_ret()) {
                                insert(ASReturn::getReturn());
                            } else {
                                insert(ASReturn::getReturn(getMapping(ret->get_operand(0))));
                            }
                            break;
                        }
                        case Instruction::br: {
                            // Move the condition inst to here.
                            auto br = dynamic_cast<BranchInst *>(inst);
                            if (br->is_cond_br()) {
                                auto cond = br->get_condition();
                                if (auto ci = dynamic_cast<ConstantInt *>(cond)) {
                                    if (ci->get_value()) {
                                        insert(ASBranchInst::createBranch(getMapping<ASLabel>(br->getTrueBB())));
                                    } else {
                                        insert(ASBranchInst::createBranch(getMapping<ASLabel>(br->getFalseBB())));
                                    }
                                } else {
                                    auto cmp = dynamic_cast<CmpInst *>(cond);
                                    auto lhs = cmp->get_operand(0);
                                    auto rhs = cmp->get_operand(1);
                                    bool inverse = inverseOperandIfNeeded(lhs, rhs);
                                    insert(ASCmpInst::createASMCmp(getMapping(lhs), getMapping(rhs)));
                                    switch (cmp->get_cmp_op()) {
                                        case CmpInst::EQ:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), ASBranchInst::CondEQ));
                                            break;
                                        case CmpInst::NE:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), ASBranchInst::CondNE));
                                            break;
                                        case CmpInst::GT:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), inverse ? ASBranchInst::CondLE : ASBranchInst::CondGT));
                                            break;
                                        case CmpInst::GE:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), inverse ? ASBranchInst::CondLT : ASBranchInst::CondGE));
                                            break;
                                        case CmpInst::LT:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), inverse ? ASBranchInst::CondGE : ASBranchInst::CondLT));
                                            break;
                                        case CmpInst::LE:
                                            insert(ASBranchInst::createCondBranch(getMapping<ASLabel>(br->getTrueBB()), inverse ? ASBranchInst::CondGT : ASBranchInst::CondLE));
                                            break;
                                    }
                                    insert(ASBranchInst::createBranch(getMapping<ASLabel>(br->getFalseBB())));
                                }
                            }
                            break;
                        }
                        case Instruction::add:
                        case Instruction::mul: {
                            auto lhs = inst->get_operand(0);
                            auto rhs = inst->get_operand(1);
                            inverseOperandIfNeeded(lhs, rhs);
                            if (inst->is_mul()) {
                                insertAndAddToMapping(inst, ASBinaryInst::createASMMul(getMapping(lhs), getMapping(rhs)));
                            }
                            else {
                                insertAndAddToMapping(inst, ASBinaryInst::createASMAdd(getMapping(lhs), getMapping(rhs)));
                            }
                            break;
                        }
                        case Instruction::sub:
                        case Instruction::sdiv: {
                            auto lhs = inst->get_operand(0);
                            auto rhs = inst->get_operand(1);
                            bool inv = inverseOperandIfNeeded(lhs, rhs);
                            if (inv) {
                                // 1 - a => mov r1, a; sub r2, r1, a
                                auto mov = ASUnaryInst::createASMMov(getMapping(rhs));
                                insert(mov);
                                if (inst->get_instr_type() == Instruction::sub) {
                                    insertAndAddToMapping(inst, ASBinaryInst::createASMSub(mov, getMapping(lhs)));
                                }
                                else {
                                    insertAndAddToMapping(inst, ASBinaryInst::createASMDiv(mov, getMapping(lhs)));
                                }
                            } else {
                                insert(ASBinaryInst::createASMSub(getMapping(lhs), getMapping(rhs)));
                            }
                            break;
                        }
                        case Instruction::mod: {
                            // TODO: Fast up when one of the operand is a constant
                            // A naive impl of mod: a % b = a - (a / b) * b
                            // 如果Op1是常数则需要mov指令
                            auto a = getMapping(inst->get_operand(0));
                            auto b = getMapping(inst->get_operand(1));
                            if (auto c_a = dynamic_cast<ASConstant *>(a)) {
                                a = insert(ASUnaryInst::createASMMov(c_a));
                            }
                            auto adb = insert(ASBinaryInst::createASMDiv(a, b));
                            auto apb = insert(ASBinaryInst::createASMMul(adb, b));
                            insertAndAddToMapping(inst, ASBinaryInst::createASMSub(a, apb));
                            break;
                        }
                        /**
                         * Alloca 语句不生成可执行的代码而仅在栈上分配一段空间，将空间的基址存入mapping中
                         */
                        case Instruction::alloca: {
                            auto al = dynamic_cast<AllocaInst *>(inst);
                            std::vector<int> dim;
                            Type *ty = al->get_alloca_type();
                            getDimensionByType(dim, ty);
                            int sz = 1;
                            for (auto i: dim) {
                                sz *= i;
                            }
                            int sp_offset_base = _current_func->allocStack(inst, sz);
                            _mapping[inst] = ASAlloca::getAlloca(sz, sp_offset_base);
                            break;
                        }
                        case Instruction::load: {
                            auto ld = dynamic_cast<LoadInst *>(inst);
                            auto from = ld->get_operand(0);
                            if (auto gv = dynamic_cast<GlobalVariable *>(from)) {
                                // Global int
                                auto ld_label = ASLoadInst::createLoad(getGlobalValue(gv));
                                insert(ld_label);
                                insertAndAddToMapping(inst, ASLoadInst::createLoad(ld_label));
                            }
                            else {
                                // GEP
                                auto gep = dynamic_cast<GetElementPtrInst *>(from);
                                auto offset = getMapping(gep);
                                Value *top = gep;
                                while (auto gep_ori = dynamic_cast<GetElementPtrInst *>(top)) {
                                    top = gep_ori->get_operand(0);
                                }
                                // 偏移量 x 4
                                if (auto off_c = dynamic_cast<ASConstant *>(offset)) {
                                    offset = ASConstant::getConstant(off_c->getValue() * 4);
                                } else {
                                    offset = insert(ASBinaryInst::createASMLsl(offset, ASConstant::getConstant(2)));
                                }
                                // 获取到原本的模式，进行判断
                                // 若是全局数组则
                                if (auto glb_v = dynamic_cast<GlobalVariable *>(top)) {
                                    // load r1, =LabelName
                                    auto ld_label = insert(ASLoadInst::createLoad(getGlobalValue(glb_v)));
                                    // load r2, [r1, rm/Imm]
                                    insertAndAddToMapping(inst, ASLoadInst::createLoad(ld_label, offset));
                                }
                                // 参数变量
                                else if (auto a = dynamic_cast<Argument *>(top)) {
                                    // 直接Load即可
                                    // TODO: Argument -> ASArgument map
                                    insertAndAddToMapping(inst, ASLoadInst::createLoad(getCurrentFunction()->getArgument(a), offset));
                                }
                                // Alloca出来的数组
                                else if (auto al = dynamic_cast<AllocaInst *>(top)) {
                                    // 计算相对于SP的偏移量
                                    auto alloca = getMapping<ASAlloca>(al);
                                    if (auto c = dynamic_cast<ASConstant *>(offset)) {
                                        offset = ASConstant::getConstant(c->getValue() + alloca->getBase());
                                    } else {
                                        offset = insert(ASBinaryInst::createASMAdd(offset, ASConstant::getConstant(alloca->getSize())));
                                    }
                                    insertAndAddToMapping(inst, ASLoadInst::createSpLoad(offset));
                                }
                            }
                            break;
                        }
                        case Instruction::store:{
                            auto st = dynamic_cast<StoreInst *>(inst);
                            auto from = st->get_operand(1);
                            auto data = getMapping(st->get_operand(0));
                            if (auto co = dynamic_cast<ASConstant *>(data)) {
                                data = insert(ASUnaryInst::createASMMov(co));
                            }
                            if (auto gv = dynamic_cast<GlobalVariable *>(from)) {
                                // Global int
                                auto ld_label = ASLoadInst::createLoad(getGlobalValue(gv));
                                insert(ld_label);
                                insertAndAddToMapping(inst, ASStoreInst::createStore(data, ld_label));
                            }
                            else {
                                // GEP
                                auto gep = dynamic_cast<GetElementPtrInst *>(from);
                                auto offset = getMapping(gep);
                                Value *top = gep;
                                while (auto gep_ori = dynamic_cast<GetElementPtrInst *>(top)) {
                                    top = gep_ori->get_operand(0);
                                }
                                // 偏移量 x 4
                                if (auto off_c = dynamic_cast<ASConstant *>(offset)) {
                                    offset = ASConstant::getConstant(off_c->getValue() * 4);
                                } else {
                                    offset = insert(ASBinaryInst::createASMLsl(offset, ASConstant::getConstant(2)));
                                }
                                // 获取到原本的模式，进行判断
                                // 若是全局数组则
                                if (auto glb_v = dynamic_cast<GlobalVariable *>(top)) {
                                    // load r1, =LabelName
                                    auto ld_label = insert(ASLoadInst::createLoad(getGlobalValue(glb_v)));
                                    // load r2, [r1, rm/Imm]
                                    insertAndAddToMapping(inst, ASStoreInst::createStore(data, ld_label, offset));
                                }
                                    // 参数变量
                                else if (auto a = dynamic_cast<Argument *>(top)) {
                                    // 直接Load即可
                                    // TODO: Argument -> ASArgument map
                                    insertAndAddToMapping(inst, ASStoreInst::createStore(data, getCurrentFunction()->getArgument(a), offset));
                                }
                                    // Alloca出来的数组
                                else if (auto al = dynamic_cast<AllocaInst *>(top)) {
                                    // 计算相对于SP的偏移量
                                    auto alloca = getMapping<ASAlloca>(al);
                                    if (auto c = dynamic_cast<ASConstant *>(offset)) {
                                        offset = ASConstant::getConstant(c->getValue() + alloca->getBase());
                                    } else {
                                        offset = insert(ASBinaryInst::createASMAdd(offset, ASConstant::getConstant(alloca->getSize())));
                                    }
                                    insertAndAddToMapping(inst, ASStoreInst::createSpStore(data, offset));
                                }
                            }
                            break;
                        }
                        case Instruction::cmp:
                            // Pass
                            // 所有CMP语句都被复制到了Cond Branch指令前，无需再次转换
                            break;
                        case Instruction::phi:
                            // 为了防止引用到未访问的BB，此处仅插入语句而不添加Value/Block
                            insertAndAddToMapping(inst, ASPhiInst::getPhi());
                            break;
                        case Instruction::call: {
                            auto call = dynamic_cast<CallInst *>(inst);
                            auto callee = getMapping<ASFunction>(call->get_operand(0));
                            std::vector<ASValue *> params;
                            for (int i = 1; i < call->get_num_operand(); ++i) {
                                params.push_back(getMapping(call->get_operand(i)));
                            }
                            auto ci = ASFunctionCall::getCall(callee, params);
                            if (ci->isVoid()) {
                                insert(ci);
                            } else {
                                insertAndAddToMapping(inst, ci);
                            }
                            break;
                        }
                        case Instruction::getelementptr: {
                            // Calculate the offset value
                            // GEP %alloca %1 %2 %3...  => 计算 %1 * dim 1 + %2 * dim 2 +...，调用时使用 Load Rd, [Sp, Rm/Imm]
                            // GEP %global %1 %2 %3...  => 计算 %1 * dim 1 +... 调用时使用 1. Load r1, =%global 2. Load Rd, [r1, Rm/Imm]
                            // 无论是哪种情况，我们都只需要计算偏移量即可。
                            // 如果计算得到的偏移量是一个常数，则直接传入的是ASConstant而不是ASBinaryInst
                            auto gep = dynamic_cast<GetElementPtrInst *>(inst);
                            // 若 ptr 是 i32* 则只能有一个参数offset，计算的是 ptr + offset * 4. Argument型
                            if (gep->get_operand(0)->get_type()->get_pointer_element_type()->is_int32_type()) {
                                // 判断是来自GEP还是来自Argument
                                if (dynamic_cast<Argument *>(gep->get_operand(0))) {
                                    // 直接将偏移值存入mapping中
                                    _mapping[inst] = getMapping(gep->get_operand(1));
                                } else {
                                    // 偏移量相加，注意常量
                                    auto ori = getMapping(gep->get_operand(0));
                                    auto off = gep->get_operand(1);
                                    // 两个操作数都是常量，直接计算
                                    if (auto off_c = dynamic_cast<ConstantInt *>(off)) {
                                        if (auto constant = dynamic_cast<ASConstant *>(ori)) {
                                            _mapping[inst] = ASConstant::getConstant(off_c->get_value() + constant->getValue());
                                            continue;
                                        }
                                    }
                                    auto off_c = getMapping(off);
                                    if (dynamic_cast<ASConstant *>(ori)) std::swap(ori, off_c);
                                    insertAndAddToMapping(inst, ASBinaryInst::createASMAdd(ori, off_c));
                                }
                            }
                            // 若 ptr 是 Array*
                            else {
                                std::vector<int> dim;
                                getDimensionByType(dim, gep->get_operand(0)->get_type()->get_pointer_element_type());
                                std::vector<int> accumulate_offset(dim.size());
                                accumulate_offset.back() = 1;
                                for (int i = (int) dim.size() - 1; i > 0; --i) {
                                    accumulate_offset[i-1] = accumulate_offset[i] * dim[i];
                                }
                                int const_offset = 0;
                                std::vector<ASBinaryInst *> var_offset;
                                // 先把当前GEP内的总偏移计算出来，注意区分常量与变量
                                for (int i = 2; i < gep->get_num_operand(); ++i) {
                                    auto idx = gep->get_operand(i);
                                    if (auto c = dynamic_cast<ConstantInt *>(idx)) {
                                        const_offset += c->get_value() * accumulate_offset[i - 2];
                                    }
                                    else {
                                        var_offset.push_back(ASBinaryInst::createASMMul(getMapping(idx), ASConstant::getConstant(accumulate_offset[i - 1])));
                                    }
                                }
                                // 判断是否是顶层GEP
                                if (!dynamic_cast<GetElementPtrInst *>(gep->get_operand(0))) {
                                    // 指向的是Alloca或全局变量

                                    // 判断是否完全是整数
                                    if (var_offset.empty()) {
                                        _mapping[inst] = ASConstant::getConstant(const_offset);
                                    } else {
                                        auto v = var_offset[0];
                                        insert(v);
                                        for (int i = 1; i < var_offset.size(); ++i) {
                                            insert(var_offset[i]);
                                            v = ASBinaryInst::createASMAdd(v, var_offset[i]);
                                            insert(v);
                                        }
                                        // 如果常量不为0也要加上
                                        if (const_offset != 0) {
                                            insertAndAddToMapping(inst, ASBinaryInst::createASMAdd(v, ASConstant::getConstant(const_offset)));
                                        } else {
                                            _mapping[inst] = v;
                                        }
                                    }
                                } else {
                                    // 指向GEP
                                    auto parent_gep_val = getMapping(gep->get_operand(0));
                                    // 判断是否完全是整数
                                    if (var_offset.empty()) {
                                        if (auto gep_c = dynamic_cast<ASConstant *>(parent_gep_val)) {
                                            _mapping[inst] = ASConstant::getConstant(const_offset + gep_c->getValue());
                                        } else {
                                            insertAndAddToMapping(inst, ASBinaryInst::createASMAdd(parent_gep_val, ASConstant::getConstant(const_offset)));
                                        }
                                    } else {
                                        if (auto gep_c = dynamic_cast<ASConstant *>(parent_gep_val)) {
                                            const_offset += gep_c->getValue();
                                        } else {
                                            // 直接加上
                                            insert(var_offset[0]);
                                            var_offset[0] = ASBinaryInst::createASMAdd(var_offset[0], parent_gep_val);
                                        }
                                        auto v = var_offset[0];
                                        insert(v);
                                        for (int i = 1; i < var_offset.size(); ++i) {
                                            insert(var_offset[i]);
                                            v = ASBinaryInst::createASMAdd(v, var_offset[i]);
                                            insert(v);
                                        }
                                        // 如果常量不为0也要加上
                                        if (const_offset != 0) {
                                            insertAndAddToMapping(inst, ASBinaryInst::createASMAdd(v, ASConstant::getConstant(const_offset)));
                                        } else {
                                            _mapping[inst] = v;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case Instruction::zext:
                            // Do nothing, map the incoming value to inst
                            _mapping[inst] = getMapping(inst->get_operand(0));
                            break;
                    }
                }
            }
            // Fix Phi instructions
            for (auto bb: f->get_basic_blocks()) {
                for (auto inst: bb->get_instructions()) {
                    if (auto phi = dynamic_cast<PhiInst *>(inst)) {
                        auto asm_phi = getMapping<ASPhiInst>(phi);
                        for (auto pair: phi->getValueBBPair()) {
                            asm_phi->addPhiPair(getMapping<ASBlock>(pair.second), getMapping(pair.first));
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }
}