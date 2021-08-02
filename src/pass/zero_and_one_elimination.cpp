#include "pass/zero_and_one_elimination.h"


void ZERO_ONE_Eliminate::run() {
    for(auto fun: m_->get_functions()){
        if(fun->is_declaration()) continue;
        for(auto bb: fun->get_basic_blocks()){
            std::vector<Instruction *> wait_delete;
            for(auto instr: bb->get_instructions()){
                if(instr->is_add()){
                    auto lhs = instr->get_operand(0);
                    auto rhs = instr->get_operand(1);
                    auto const_lhs = dynamic_cast<ConstantInt *>(lhs);
                    auto const_rhs = dynamic_cast<ConstantInt *>(rhs);
                    if(const_lhs && const_lhs->get_value() == 0){
                        instr->replace_all_use_with(rhs);
                        wait_delete.push_back(instr);
                    }
                    else if(const_rhs && const_rhs->get_value() == 0){
                        instr->replace_all_use_with(lhs);
                        wait_delete.push_back(instr);
                    }
                }
                else if(instr->is_sub()){
                    auto lhs = instr->get_operand(0);
                    auto rhs = instr->get_operand(1);
                    auto const_rhs = dynamic_cast<ConstantInt *>(rhs);
                    if(const_rhs && const_rhs->get_value() == 0){
                        instr->replace_all_use_with(lhs);
                        wait_delete.push_back(instr);
                    }
                }
                else if(instr->is_mul()){
                    auto lhs = instr->get_operand(0);
                    auto rhs = instr->get_operand(1);
                    auto const_lhs = dynamic_cast<ConstantInt *>(lhs);
                    auto const_rhs = dynamic_cast<ConstantInt *>(rhs);
                    if(const_lhs && const_lhs->get_value() == 0){
                        instr->replace_all_use_with(ConstantInt::get(0, m_));
                        wait_delete.push_back(instr);
                    }
                    else if(const_lhs && const_lhs->get_value() == 1){
                        instr->replace_all_use_with(rhs);
                        wait_delete.push_back(instr);
                    }
                    else if(const_rhs && const_rhs->get_value() == 0){
                        instr->replace_all_use_with(ConstantInt::get(0, m_));
                        wait_delete.push_back(instr);
                    }
                    else if(const_rhs && const_rhs->get_value() == 1){
                        instr->replace_all_use_with(lhs);
                        wait_delete.push_back(instr);
                    }
                }
                else if(instr->is_div()){
                    auto lhs = instr->get_operand(0);
                    auto rhs = instr->get_operand(1);
                    auto const_rhs = dynamic_cast<ConstantInt *>(rhs);
                    if(const_rhs && const_rhs->get_value() == 1){
                        instr->replace_all_use_with(lhs);
                        wait_delete.push_back(instr);
                    }
                }
//                else if(instr->is_rem()){
//
//                }
            }
            for(auto instr: wait_delete){
                bb->delete_instr(instr);
            }
        }
    }
}