#include "pass/loop_expansion.h"

bool  simplify = true;

bool cmp_op(int op1, int op2, CmpInst::CmpOp cmp_op){
    switch (cmp_op) {
        case CmpInst::CmpOp::EQ:
            return op1 == op2;
        case CmpInst::CmpOp::GE:
            return op1 >= op2;
        case CmpInst::CmpOp::GT:
            return op1 > op2;
        case CmpInst::CmpOp::NE:
            return op1 != op2;
        case CmpInst::CmpOp::LE:
            return op1 <= op2;
        case CmpInst::CmpOp::LT:
            return op1 < op2;
    }
    return false;
}

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
                        target.insert(update_val);
                        std::cout << update_val->get_name() << std::endl;
                        target_phi[update_val] = new_phi_node;
                        ins2node[instr] = new_phi_node;
                        base.insert(instr);
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
                bool const_loop = true;
                while(const_loop){
                    auto decesion_node = ins2node[decision_val];
                    int op1_val, op2_val;
                    auto cmp_inst = dynamic_cast<CmpInst *>(decision_val);
                    auto cmp_type = cmp_inst->get_cmp_op();
                    auto op1 = cmp_inst->get_operand(0);
                    auto op2 = cmp_inst->get_operand(1);
                    auto const_op1 = dynamic_cast<ConstantInt *>(op1);
                    auto const_op2 = dynamic_cast<ConstantInt *>(op2);
                    auto phi_op1 = dynamic_cast<Phi_Node *>(ins2node[op1]);
                    auto phi_op2 = dynamic_cast<Phi_Node *>(ins2node[op2]);
                    if(const_op1){
                        op1_val = const_op1->get_value();
                    }
                    else if(phi_op1){
                        op1_val = phi_op1->get_val();
                    }
                    else{
                        auto op_node_1 = dynamic_cast<Judge_Node *>(ins2node[op1]);
                        op1_val = op_node_1->calculate_judge_value(ins2node);
                    }
                    if(const_op2){
                        op2_val = const_op2->get_value();
                    }
                    else if(phi_op2){
                        op2_val = phi_op2->get_val();
                    }
                    else{
                        auto op_node_2 = dynamic_cast<Judge_Node *>(ins2node[op2]);
                        op2_val = op_node_2->calculate_judge_value(ins2node);
                    }
                    std::cout << op1_val << " " << op2_val << std::endl;
                    if(cmp_op(op1_val, op2_val, cmp_type)){
                        expansion_time ++;
                    }
                    else{
                        break;
                    }
                    const_loop = update_base_value(loop_block);
                }
                std::cout << "expansion time: " << expansion_time << std::endl;
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
        default:
            return 0;
            break;
    }
}

bool LoopExpansion::update_base_value(BasicBlock *loop_block) {
    for(auto instr: loop_block->get_instructions()){
        if(instr->isBinary()){
//            std::cout << instr->get_name() << std::endl;
            auto bin_instr = dynamic_cast<BinaryInst *>(instr);
            auto op1 = instr->get_operand(0);
            auto op2 = instr->get_operand(1);
            int op1_val, op2_val;
            auto const_op1 = dynamic_cast<ConstantInt *>(op1);
            auto const_op2 = dynamic_cast<ConstantInt *>(op2);
            auto phi_op1 = dynamic_cast<Phi_Node *>(ins2node[op1]);
            auto phi_op2 = dynamic_cast<Phi_Node *>(ins2node[op2]);
            if(const_op1){
                op1_val = const_op1->get_value();
            }
            else if(phi_op1){
                op1_val = phi_op1->get_val();
            }
            else if(loop_vars.find(op1) != loop_vars.end()){
                op1_val = loop_vars[op1];
            }
            else{
                return false;
            }
            if(const_op2){
                op2_val = const_op2->get_value();
            }
            else if(phi_op2){
                op2_val = phi_op2->get_val();
            }
            else if(loop_vars.find(op2) != loop_vars.end()){
                op2_val = loop_vars[op2];
            }
            else{
                return false;
            }
            int res = 0;
            switch (bin_instr->get_instr_type()) {
                case BinaryInst::OpID::add:
                    res = op1_val + op2_val;
                    loop_vars[instr] = res;
                    break;
                case BinaryInst::OpID::sub:
                    res = op1_val - op2_val;
                    loop_vars[instr] = res;
                    break;
                case BinaryInst::OpID::mul:
                    res = op1_val * op2_val;
                    loop_vars[instr] = res;
                    break;
                case BinaryInst::OpID::sdiv:
                    res = op1_val / op2_val;
                    loop_vars[instr] = res;
                    break;
                default:
                    return false;
            }
//            for(auto tar: target){
//                std::cout << tar->get_name() << std::endl;
//            }
            if(target.find(instr) != target.end()){
                auto phi_node = target_phi[instr];
                phi_node->set_val(res);
            }
        }
    }
    return true;
}