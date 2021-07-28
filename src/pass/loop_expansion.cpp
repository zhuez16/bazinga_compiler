#include "pass/loop_expansion.h"

bool  simplify = true;

Function *curfun;

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
    auto *loop_search = new LoopSearch(m_);
    loop_search->run();
    for(auto fun: m_->get_functions()){
        curfun = fun;
        for(auto loop: loop_search->get_loop(fun)){
            ins2node.clear();
            loop_vars.clear();
            target.clear();
            target_phi.clear();
            base.clear();
            phi_value_stack.clear();
            simplify = true;
            if(loop->get_loop().size() == 2){
                // consider the simpliest case
                auto bb_set = loop->get_loop();
                // find judge block and loop block
                auto judge_block = loop->get_loop_entry();
//                std::cout << judge_block->get_name() << std::endl;
                BasicBlock* loop_block = nullptr;
                for(auto succ: judge_block->get_succ_basic_blocks()){
                    if(bb_set.find(succ) != bb_set.end()){
                        loop_block = succ;
                        break;
                    }
                }
//                std::cout << loop_block->get_name() << std::endl;
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
//                        std::cout << update_val->get_name() << std::endl;
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
//                            std::cout << "555" << std::endl;
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
                        auto const_op1 = dynamic_cast<ConstantInt *>(op1);
                        auto const_op2 = dynamic_cast<ConstantInt *>(op2);
                        if((!const_op1 && ins2node.find(op1) == ins2node.end())
                        || (!const_op2 && ins2node.find(op2) == ins2node.end())){
//                            std::cout << "555" << std::endl;
                            simplify = false;
                            break;
                        }
                        auto new_judge_node = new Judge_Node(instr, op1, op2, cmp_instr->get_instr_type());
                        ins2node[instr] = new_judge_node;
                        decision_val = instr;
                        cmp_id = dynamic_cast<CmpInst *>(instr)->get_cmp_op();
                    }
                    else if(instr->is_br()){
                        continue;
                    }
                    else{
                        simplify = false;
                        break;
                    }
                }
//                std::cout << "loop debug 1" << std::endl;
                if(!simplify) continue;
//                std::cout << "loop debug 1" << std::endl;
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
                    }
                    else if(instr->is_call()){
                        auto call_instr = dynamic_cast<CallInst *>(instr);
                        auto fun = dynamic_cast<Function *>(call_instr->get_operand(0));
                        if(fun == curfun){
                            simplify = false;
                            break;
                        }
                        continue;
                    }
                }
                if(!simplify) continue;
                int expansion_time = 0;
                bool const_loop = true;
                bool expansion_simplify = true;
                while(const_loop){
                    std::map<Value *, int> phi_value;
                    for(auto phi_instr: base){
                        auto phi_node = dynamic_cast<Phi_Node *>(ins2node[phi_instr]);
                        phi_value[phi_instr] = phi_node->get_val();
                    }
                    phi_value_stack.push_back(phi_value);
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
//                    std::cout << op1_val << " " << op2_val << std::endl;
                    const_loop = update_base_value(loop_block);
                    if(expansion_time == 0 && !const_loop){
                        expansion_simplify = false;
                        break;
                    }
                    if(cmp_op(op1_val, op2_val, cmp_type)){
                        expansion_time ++;
                    }
                    else{
                        break;
                    }
                }
                if(expansion_simplify && expansion_time != 0){
//                    std::cout << "expansion time: " << expansion_time << std::endl;
                    auto new_bb = unroll_loop(expansion_time, judge_block, loop_block);
                    for(auto x: phi_value_stack[expansion_time]){
                        x.first->replace_all_use_with(ConstantInt::get(x.second, m_));
                    }
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
        case BinaryInst::OpID::sub:
            return val_1 - val_2;
//            this->set_node_val(val_1 + val_2);
        case BinaryInst::OpID::mul:
            return val_1 * val_2;
//            this->set_node_val(val_1 + val_2);
        case BinaryInst::OpID::sdiv:
            return val_1 / val_2;
//            this->set_node_val(val_1 + val_2);
        case BinaryInst::OpID::mod:
            return val_1 % val_2;
        default:
            return 0;
    }
}

bool LoopExpansion::update_base_value(BasicBlock *loop_block) {
    // if target num != num we can get, return false
    int target_num = 0;
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
                continue;
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
                continue;
            }
            int res;
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
                case BinaryInst::OpID::mod:
                    res = op1_val % op2_val;
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
                target_num ++;
                if(target_num == target.size()){
                    return true;
                }
            }
        }
    }
    return false;
}

BasicBlock * LoopExpansion::unroll_loop(int expansion_time, BasicBlock *judge_bb, BasicBlock *loop_bb) {
    auto bb = BasicBlock::create(m_, "", curfun);
    std::map<Value *, Value *> loop_map;
    for(int i = 0; i < expansion_time; i++){
        auto phi_val = this->phi_value_stack[i];
        std::set<Value *> loop_values;
        for(auto base: this->base){
            loop_values.insert(base);
        }
        for(auto instr: loop_bb->get_instructions()){
            if(instr->is_add()){
                auto add_instr = dynamic_cast<BinaryInst *>(instr);
                auto op0 = add_instr->get_operand(0);
                auto op1 = add_instr->get_operand(1);
                auto new_add = BinaryInst::create_add(op0, op1, bb, m_);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_add);
                    new_add->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_add);
                    new_add->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_add);
                    new_add->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_add);
                    new_add->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_add;
            }
            else if(instr->is_sub()){
                auto sub_instr = dynamic_cast<BinaryInst *>(instr);
                auto op0 = sub_instr->get_operand(0);
                auto op1 = sub_instr->get_operand(1);
                auto new_sub = BinaryInst::create_sub(op0, op1, bb, m_);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_sub);
                    new_sub->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_sub);
                    new_sub->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_sub);
                    new_sub->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_sub);
                    new_sub->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_sub;
            }
            else if(instr->is_mul()){
                auto mul_instr = dynamic_cast<BinaryInst *>(instr);
                auto op0 = mul_instr->get_operand(0);
                auto op1 = mul_instr->get_operand(1);
                auto new_mul = BinaryInst::create_mul(op0, op1, bb, m_);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_mul);
                    new_mul->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_mul);
                    new_mul->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_mul);
                    new_mul->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_mul);
                    new_mul->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_mul;
            }
            else if(instr->is_div()){
                auto div_instr = dynamic_cast<BinaryInst *>(instr);
                auto op0 = div_instr->get_operand(0);
                auto op1 = div_instr->get_operand(1);
                auto new_div = BinaryInst::create_sdiv(op0, op1, bb, m_);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_div);
                    new_div->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_div);
                    new_div->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_div);
                    new_div->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_div);
                    new_div->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_div;
            }
            else if(instr->is_rem()){
                auto rem_instr = dynamic_cast<BinaryInst *>(instr);
                auto op0 = rem_instr->get_operand(0);
                auto op1 = rem_instr->get_operand(1);
                auto new_rem = BinaryInst::create_mod(op0, op1, bb, m_);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_rem);
                    new_rem->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_rem);
                    new_rem->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_rem);
                    new_rem->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_rem);
                    new_rem->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_rem;
            }
            else if(instr->is_gep()){
                auto gep_instr = dynamic_cast<GetElementPtrInst *>(instr);
                std::vector<Value *> idx;
                auto ptr = gep_instr->get_operand(0);
                for(int j = 1; j < gep_instr->get_operands().size(); j++){
                    idx.push_back(gep_instr->get_operand(j));
                }
                auto new_gep = GetElementPtrInst::create_gep(ptr, idx, bb);
                for(int j = 0; j < gep_instr->get_operands().size(); j++){
                    auto op_val = gep_instr->get_operand(j);
                    if(phi_val.find(op_val) != phi_val.end()){
                        op_val->remove_use(new_gep);
                        new_gep->set_operand(j, ConstantInt::get(phi_val[op_val], m_));
                    }
                    else if(loop_values.find(op_val) != loop_values.end()){
                        op_val->remove_use(new_gep);
                        new_gep->set_operand(j, loop_map[op_val]);
                    }
                }
                loop_values.insert(instr);
                loop_map[instr] = new_gep;
            }
            else if(instr->is_store()){
                auto store_instr = dynamic_cast<StoreInst *>(instr);
                auto op0 = store_instr->get_operand(0);
                auto op1 = store_instr->get_operand(1);
                auto new_store = StoreInst::create_store(op0, op1, bb);
                if(phi_val.find(op0) != phi_val.end()){
                    op0->remove_use(new_store);
                    new_store->set_operand(0, ConstantInt::get(phi_val[op0], m_));
                }
                else if(loop_values.find(op0) != loop_values.end()){
                    op0->remove_use(new_store);
                    new_store->set_operand(0, loop_map[op0]);
                }
                if(phi_val.find(op1) != phi_val.end()){
                    op1->remove_use(new_store);
                    new_store->set_operand(1, ConstantInt::get(phi_val[op1], m_));
                }
                else if(loop_values.find(op1) != loop_values.end()){
                    op1->remove_use(new_store);
                    new_store->set_operand(1, loop_map[op1]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_store;
            }
            else if(instr->is_load()){
                auto load_instr = dynamic_cast<LoadInst *>(instr);
                auto load_ptr = load_instr->get_operand(0);
                auto new_load = LoadInst::create_load(load_instr->get_load_type(), load_ptr, bb);
                if(phi_val.find(load_ptr) != phi_val.end()){
                    load_ptr->remove_use(new_load);
                    new_load->set_operand(0, ConstantInt::get(phi_val[load_ptr], m_));
                }
                else if(loop_values.find(load_ptr) != loop_values.end()){
                    load_ptr->remove_use(new_load);
                    new_load->set_operand(0, loop_map[load_ptr]);
                }
                loop_values.insert(instr);
                loop_map[instr] = new_load;
            }
            else if(instr->is_br()){
                continue;
            }
            else if(instr->is_call()){
                auto call_instr = dynamic_cast<CallInst *>(instr);
                std::vector<Value *> args;
                for(auto op: call_instr->get_operands()){
//                    std::cout << op->get_name() << std::endl;
                }
                for(int k = 1; k < call_instr->get_operands().size(); k++){
                    args.push_back(call_instr->get_operand(k));
                }
                auto fun = dynamic_cast<Function *>(call_instr->get_operand(0));
                auto new_call = CallInst::create(fun, args, bb);
                for(int j = 0; j < call_instr->get_operands().size(); j++){
                    auto op_val = call_instr->get_operand(j);
                    if(phi_val.find(op_val) != phi_val.end()){
                        op_val->remove_use(new_call);
                        new_call->set_operand(j, ConstantInt::get(phi_val[op_val], m_));
                    }
                    else if(loop_values.find(op_val) != loop_values.end()){
                        op_val->remove_use(new_call);
                        new_call->set_operand(j, loop_map[op_val]);
                    }
                }
                loop_values.insert(instr);
                loop_map[instr] = new_call;
                continue;
            }
        }
    }
    std::vector<Instruction *> wait_delete_instr;
    std::vector<BasicBlock *> wait_delete_bb;
    auto judge_bb_final = judge_bb->get_terminator();
    auto loop_bb_final = loop_bb->get_terminator();
    auto target_bb = judge_bb_final->get_operand(2);
    for(auto succ: judge_bb->get_succ_basic_blocks()){
        wait_delete_bb.push_back(succ);
    }
    for(auto bb: wait_delete_bb){
        judge_bb->remove_succ_basic_block(bb);
    }
    judge_bb->remove_pre_basic_block(loop_bb);
    for(auto instr: judge_bb->get_instructions()){
        wait_delete_instr.push_back(instr);
    }
    for(auto instr: wait_delete_instr){
        judge_bb->delete_instr(instr);
    }
    auto judge_br_instr = BranchInst::create_br(bb, judge_bb);
    auto bb_br_instr = BranchInst::create_br(loop_bb, bb);
    bb->add_succ_basic_block(loop_bb);
    bb->add_pre_basic_block(judge_bb);
    loop_bb->remove_succ_basic_block(judge_bb);
    loop_bb->add_pre_basic_block(bb);
    wait_delete_instr.clear();
    wait_delete_bb.clear();
    for(auto instr: loop_bb->get_instructions()){
        wait_delete_instr.push_back(instr);
    }
    for(auto instr: wait_delete_instr){
        loop_bb->delete_instr(instr);
    }
    auto loop_br_instr = BranchInst::create_br(dynamic_cast<BasicBlock *>(target_bb), loop_bb);
    return bb;
}