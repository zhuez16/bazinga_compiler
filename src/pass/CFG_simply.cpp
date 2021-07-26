//
// Created by 万嘉诚 on 2021/7/26.
//

#include "CFG_simply.h"

void CFG_simply::run() {
    for (auto fun : m_->getFunctions())
    {
        func_ = fun;
        del_no_pre();
        del_singel_phi();
        merge_single();
        del_uncond();

        del_self_loop();
        del_no_pre();

        del_singel_phi();
    }
}

void CFG_simply::del_self_loop() {
    for (auto bb : func_->get_basic_blocks())
    {
        if ((bb->get_pre_basic_blocks().size() == 1)&&
            (bb != get_entry_block()) &&
            (*(bb->get_pre_basic_blocks().begin()) == bb))
            func_->remove(bb);
    }
}

void del_no_pre_(BasicBlock * bb) {
    if (bb->get_pre_basic_blocks().empty() && bb != func_->get_entry_block()) {
        for (auto succ_bb : bb->get_succ_basic_blocks()) {
            succ_bb->remove_pre_basic_block(bb);
            del_no_pre_(succ_bb);
        }
        func_->remove(bb);
    }
}

void CFG_simply::del_no_pre() {
    for (auto bb : func_->get_basic_blocks()) {
        del_no_pre_(bb);
    }
}

void CFG_simply::merge_single() {
    for (auto bb : func_->get_basic_blocks()) {
        if (bb->get_pre_basic_blocks().size() == 1) {
            auto pre_bb = bb->get_pre_basic_blocks().begin();
            auto br = (*pre_bb)->get_terminator();
            if (br->get_num_operand() == 1) {
                (*pre_bb)->delete_instr(br);
                for (auto instr : bb->get_instructions()) {
                    instr->set_parent(*pre_bb);
                    (*pre_bb)->add_instruction(instr);
                }
                (*pre_bb)->remove_succ_basic_block(bb);
                for (auto succ_bb : bb->get_succ_basic_blocks()) {
                    (*pre_bb)->add_succ_basic_block(succ_bb);
                    succ_bb->get_pre_basic_blocks().remove(bb);
                    succ_bb->add_pre_basic_block(*pre_bb);
                }
                bb->replace_all_use_with(*pre_bb);
                func_->remove(bb);
            }
        }
    }
}

void CFG_simply::del_singel_phi() {
    for (auto bb : func_->get_basic_blocks()) {
        if (bb->get_pre_basic_blocks().size() == 1 || bb == func_->get_entry_block()) {
            for (auto instr : bb->get_instructions()) {
                if (instr->is_phi()) {
                    for (auto use : instr->get_use_list())
                        static_cast<User *>(use.val_)->set_operand(
                                    use.arg_no_, instr->get_operand(0));
                    bb->delete_instr(instr);
                }
            }
        }
    }
}

void CFG_simply::del_uncond() {
    for (auto bb : func_->get_basic_blocks()) {
        if (bb->get_num_of_instr() == 1 && bb != func_->get_entry_block()) {
            auto terminator = bb->get_terminator();
            auto succ_bb = bb->get_succ_basic_blocks().begin();
            if (terminator->is_br() && terminator->get_num_operand() == 1) {
                bool if_del = true;
                for (auto pre_bb : bb->get_pre_basic_blocks()) {
                    auto br = static_cast<BranchInst *>(pre_bb->get_terminator());
                    if (br->is_cond_br()) {
                        auto target1 = br->get_operand(1);
                        auto target2 = br->get_operand(2);
                        auto target3 = static_cast<BranchInst *>(terminator)->get_operand(0);
                        if ((target1 == bb && target2 == target3) ||
                            (target2 == bb && target1 == target3)) {
                            if_del = false;
                            break;
                        }
                    } else if (br->is_cmp()) {
                        auto target1 = br->get_operand(2);
                        auto target2 = br->get_operand(3);
                        auto target3 = static_cast<BranchInst *>(terminator)->get_operand(0);
                        if ((target1 == bb && target2 == target3) ||
                            (target2 == bb && target1 == target3)) {
                            if_del = false;
                            break;
                        }
                    }
                }

                if (if_del) {
                    for (auto instr : (*succ_bb)->get_instructions()) {
                        if (instr->is_phi()) {
                            for (int i = 0; i < instr->get_num_operand(); i++) {
                                if (i % 2 == 1) {
                                    if (instr->get_operand(i) == bb) {
                                        auto idx = 0;
                                        auto val = instr->get_operand(i - 1);
                                        for (auto pre_bb : bb->get_pre_basic_blocks()) {
                                            if (idx == 0) {
                                                instr->set_operand(i, pre_bb);
                                                bb->remove_use(instr, i);
                                            } else {
                                                instr->add_operand(val);
                                                instr->add_operand(pre_bb);
                                            }
                                            idx++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    for (auto use : bb->get_use_list()) {
                        auto instr = dynamic_cast<Instruction *>(use.val_);
                        if (instr) {
                            if (instr->is_br()) {
                                static_cast<User *>(use.val_)->set_operand(use.arg_no_,
                                                                        *succ_bb);
                            }
                        }
                    }

                    (*succ_bb)->remove_pre_basic_block(bb);
                    for (auto pre_bb : bb->get_pre_basic_blocks()) {
                        pre_bb->remove_succ_basic_block(bb);
                        pre_bb->add_succ_basic_block(*succ_bb);
                        (*succ_bb)->add_pre_basic_block(pre_bb);
                    }
                    func_->remove(bb);
                }
            }
        }
    }
}