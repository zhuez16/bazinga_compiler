#include "pass/loop_expansion.h"

bool  simplify = true;

void LoopExpansion::run() {
    LoopSearch *loop_search = new LoopSearch(m_);
    loop_search->run();
    for(auto fun: m_->get_functions()){
        for(auto loop: loop_search->get_loop(fun)){
            if(loop->get_loop().size() == 2){
                // consider the simpliest case
                auto bb_set = loop->get_loop();
                // find judge block and loop block
                auto judge_block = loop->get_loop_entry();
                BasicBlock* loop_block = nullptr;
                for(auto succ: judge_block->get_succ_basic_blocks()){
                    if(bb_set.find(succ) != bb_set.end()){
                        loop_block = succ;
                        break;
                    }
                }
                auto cmp_id = CmpInst::CmpOp::EQ;
                int target_val = 0;
                Value *decision_val = nullptr;
                for(auto instr: judge_block->get_instructions()){
                    if(instr->is_phi()){
                        auto phi_instr = dynamic_cast<PhiInst *>(instr);
                        int init_val;
                        if(dynamic_cast<ConstantInt *>(phi_instr->get_operand(0))){
                            init_val = dynamic_cast<ConstantInt *>(phi_instr->get_operand(0))->get_value();
                        }
                        else{
                            simplify = false;
                        }
                        if(!simplify) break;
//                        phi_instr->get_operand(1);
                        auto update_val = phi_instr->get_operand(2);
//                        std::cout << phi_instr->get_operand(3)->get_name() << std::endl;
                        auto new_phi_node = new Phi_Node(init_val, update_val);
                        ins2node[instr] = new_phi_node;
                    }
                    else if(instr->isBinary()){
                        auto bin_instr = dynamic_cast<BinaryInst *>(instr);
                        auto op1 = bin_instr->get_operand(0);
                        auto op2 = bin_instr->get_operand(1);
                        if((!dynamic_cast<ConstantInt *>(op1) && ins2node.find(op1) == ins2node.end())
                        || (!dynamic_cast<ConstantInt *>(op2) && ins2node.find(op2) == ins2node.end())){
                            simplify = false;
                        }
                        else{
                            simplify = true;
                        }
                        if(!simplify) break;
                        auto new_judge_node = new Judge_Node(instr, op1, op2, bin_instr->get_instr_type());
                        ins2node[instr] = new_judge_node;
                    }
                    else if(instr->is_cmp()){
                        auto cmp_instr = dynamic_cast<CmpInst *>(instr);
                        auto op1 = cmp_instr->get_operand(0);
                        auto op2 = cmp_instr->get_operand(1);
                        auto new_judge_node = new Judge_Node(instr, op1, op2, cmp_instr->get_instr_type());
                        ins2node[instr] = new_judge_node;
                        decision_val = instr;
                        cmp_id = dynamic_cast<CmpInst *>(instr)->get_cmp_op();
                    }
                    else if(instr->is_br()){
                        ;
                    }
                    else{
                        simplify = false;
                        break;
                    }
                }
                if(!simplify) continue;
                for(auto instr: loop_block->get_instructions()){
                    if(instr->isBinary()){
                        auto bin_instr = dynamic_cast<BinaryInst *>(instr);
                        auto op1 = bin_instr->get_operand(0);
                        auto op2 = bin_instr->get_operand(1);
//                        if((!dynamic_cast<ConstantInt *>(op1)
//                        && ins2node.find(op1) == ins2node.end()
//                        && loop_nodes.find(op1) == loop_nodes.end())
//                        ||(!dynamic_cast<ConstantInt *>(op2)
//                        && ins2node.find(op2) == ins2node.end()
//                        && loop_nodes.find(op2) == loop_nodes.end())){
//                            simplify = false;
//                        }
//                        else{
//                            simplify = true;
//                        }
//                        if(!simplify) break;
                        auto new_loop_node = new Loop_Node(op1, op2);
                        loop_nodes[instr] = new_loop_node;
                    }
                    else{
                        // TODO:??????
                    }
                }
                int expansion_time = 0;
                while(1){
                    auto decesion_node = ins2node[decision_val];
                }
            }
        }
    }
}

int Judge_Node::calculate_judge_value(std::map<Value *, Node *> ins2node) {
    auto node_1 = ins2node[this->op1];
    auto node_2 = ins2node[this->op2];
    int val_1, val_2;
    if(dynamic_cast<Judge_Node *>(node_1)){
        auto node_ = dynamic_cast<Judge_Node *>(node_1);
        val_1 = node_->calculate_judge_value(ins2node);
    }
    else{
        auto node_ = dynamic_cast<Phi_Node *>(node_1);
        val_1 = node_->get_val();
    }
    if(dynamic_cast<Judge_Node *>(node_2)){
        auto node_ = dynamic_cast<Judge_Node *>(node_2);
        val_2 = node_->calculate_judge_value(ins2node);
    }
    else{
        auto node_ = dynamic_cast<Phi_Node *>(node_2);
        val_2 = node_->get_val();
    }
    switch (this->op) {
        case BinaryInst::OpID::add:
            return val_1 + val_2;
//            this->set_node_val(val_1 + val_2);
            break;
        case BinaryInst::OpID::sub:
            return val_1 - val_2;
//            this->set_node_val(val_1 + val_2);
            break;
        case BinaryInst::OpID::mul:
            return val_1 * val_2;
//            this->set_node_val(val_1 + val_2);
            break;
        case BinaryInst::OpID::sdiv:
            return val_1 / val_2;
//            this->set_node_val(val_1 + val_2);
            break;
        default: break;
    }
}
