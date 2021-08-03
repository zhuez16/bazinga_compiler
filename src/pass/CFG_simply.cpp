//
// Created by 万嘉诚 on 2021/7/26.
//

#include "include/pass/CFG_simply.h"

void CFG_simply::run() {
    CFG cfg = CFG(m_);
    for (auto fun : m_->get_functions())
    {
        func_ = fun;
        cfg.runOnFunction(func_);
        if(func_->is_declaration())
            continue;

        fix_succ();
        fix_pre();
        for (auto bb : func_->get_basic_blocks())
        {
            bb->get_pre_basic_blocks().unique();
            bb->get_succ_basic_blocks().unique();
        }
        fix_phi();

        del_no_pre();
        del_singel_phi();
        merge_single();

        fix_succ();
        fix_pre();
        for (auto bb : func_->get_basic_blocks())
        {
            bb->get_pre_basic_blocks().unique();
            bb->get_succ_basic_blocks().unique();
        }
        fix_phi();

        del_uncond();

        del_self_loop();
        del_no_pre();

        del_singel_phi();
    }
}

void CFG_simply::fix_phi()
{
    for (auto bb : func_->get_basic_blocks())
    {
        for (auto instr : bb->get_instructions())
            if (instr->is_phi()) {
                for (int i = 0; i < instr->get_num_operand(); i++)
                    if (i % 2 == 1) {
                        bool flag = true;
                        for(auto pre_bb : bb->get_pre_basic_blocks())
                        {
                            if(instr->get_operand(i) == pre_bb)
                            {
                                flag = false;
                                break;
                            }
                        }
                        if(flag)
                        {
                            instr->remove_operands(i-1,i);
                        }
                    }
            }
    }
}

void CFG_simply::fix_pre()
{
    std::vector<BasicBlock *>del_pre;
    for (auto bb : func_->get_basic_blocks())
    {
        del_pre.clear();
        for(auto pre_bb : bb->get_pre_basic_blocks())
        {
            bool flag = true;
            for(auto succ_bb : pre_bb->get_succ_basic_blocks())
            {
                if(bb == succ_bb)
                    flag = false;
            }
            if(flag)
                del_pre.push_back(pre_bb);
        }
        for(auto tmp : del_pre)
            bb->remove_pre_basic_block(tmp);
    }
}

void CFG_simply::fix_succ()
{
    std::vector<BasicBlock *> Succ_bb;
    for (auto bb : func_->get_basic_blocks())
    {
        Succ_bb.clear();
        auto instr = bb->get_terminator();
        if(instr== nullptr)
            continue;
        if (instr->is_br()&&instr->get_num_operand()==1) {
            auto target_bb = static_cast<BasicBlock *>(instr->get_operand(0));
            for (auto succ_bb : bb->get_succ_basic_blocks()) {
                if (succ_bb != target_bb)
                    Succ_bb.push_back(succ_bb);
            }
            for (auto succ_bb : Succ_bb) {
                bb->remove_succ_basic_block(succ_bb);
                succ_bb->remove_pre_basic_block(bb);
            }
        }
    }
}

void CFG_simply::del_self_loop() {
    bb_del.clear();
    for (auto bb : func_->get_basic_blocks())
    {
        if ((bb->get_pre_basic_blocks().size() == 1)&&
            (bb != func_->get_entry_block()) &&
            (*(bb->get_pre_basic_blocks().begin()) == bb))
            bb_del.push_back(bb);
    }
    for(auto bb : bb_del)
        func_->remove(bb);
}

void CFG_simply::del_no_pre_(BasicBlock * bb) {
    if (bb->get_pre_basic_blocks().empty() && bb != func_->get_entry_block()) {
        for (auto succ_bb : bb->get_succ_basic_blocks()) {
            succ_bb->remove_pre_basic_block(bb);
            del_no_pre_(succ_bb);
        }
        bb_del.push_back(bb);
    }
}

void CFG_simply::del_no_pre() {
    bb_del.clear();
    for (auto bb : func_->get_basic_blocks()) {
        del_no_pre_(bb);
    }
    for(auto bb : bb_del)
        func_->remove(bb);
}

void CFG_simply::merge_single() {
    bb_del.clear();
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
                bb_del.push_back(bb);
            }
        }
    }
    for(auto bb : bb_del)
        func_->remove(bb);
}

void CFG_simply::del_singel_phi() {
    std::vector<Instruction*> instr_del;
    for (auto bb : func_->get_basic_blocks()) {
        instr_del.clear();
        if (bb->get_pre_basic_blocks().size() == 1 || bb == func_->get_entry_block()) {
            for (auto instr : bb->get_instructions()) {
                if (instr->is_phi()) {
                    for (auto use : instr->get_use_list())
                        static_cast<User *>(use.val_)->set_operand(
                                    use.arg_no_, instr->get_operand(0));
                    instr_del.push_back(instr);
                }
            }
        }
        for(auto instr : instr_del)
            bb->delete_instr(instr);
    }
}

void CFG_simply::del_uncond() {
    bb_del.clear();
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
                                            if(pre_bb == bb)
                                                continue;
                                            if (idx == 0) {
                                                instr->set_operand(i, pre_bb);
                                                bb->remove_use(instr);
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
                    auto tmp = bb->get_use_list();
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
                    bb_del.push_back(bb);
                }
            }
        }
    }
    for(auto bb : bb_del)
        func_->remove(bb);
}