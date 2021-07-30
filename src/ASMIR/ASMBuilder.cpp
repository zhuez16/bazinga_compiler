//
// Created by 顾超 on 2021/7/30.
//

#include "ASMIR/ASMBuilder.h"

void ASMBuilder::build(Module *m) {
    _mapping.clear();
    // Prepare all labels, so that br inst can find its destination
    for (auto f: m->get_functions()) {
        if (!f->is_declaration()) {
            // Step 1. Add all function to mapping
            auto asFunc = ASFunction::createFunction(f->get_name());
            _mapping[f] = asFunc;
            // Step 2. Add function arguments to mapping
            // At this time we assume that we have infinity registers, so all params are passed through register
            for (auto arg: f->get_args()) {
                _mapping[arg] = ASArgument::createArgument();
            }
            // Step 3. Add all basic blocks to mapping
            for (auto bb: f->get_basic_blocks()) {
                _mapping[bb] = ASBlock::createBlock(asFunc, bb->get_name());
            }
        } else {
            _mapping[f] = ASFunction::createFunction(f->get_name());
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
                                auto ret_val = ret->get_operand(0);
                                if (auto c = dynamic_cast<ConstantInt *>(ret_val)) {
                                    auto op2 = ASOperand2::getOperand2(c->get_value());
                                    auto ins = ASUnaryInst::createASMMov(op2);
                                    insert(ins);
                                }
                                // TODO
                            }
                            insert(ASBranchInst::createReturnBranch());
                            break;
                        }
                        case Instruction::br:
                            break;
                        case Instruction::add:
                            break;
                        case Instruction::sub:
                            break;
                        case Instruction::mul:
                            break;
                        case Instruction::sdiv:
                            break;
                        case Instruction::mod:
                            break;
                        case Instruction::alloca:
                            break;
                        case Instruction::load:
                            break;
                        case Instruction::store:
                            break;
                        case Instruction::cmp:
                            break;
                        case Instruction::phi:
                            break;
                        case Instruction::call:
                            break;
                        case Instruction::getelementptr:
                            break;
                        case Instruction::zext:
                            break;
                    }
                }
            }

        }
    }
}