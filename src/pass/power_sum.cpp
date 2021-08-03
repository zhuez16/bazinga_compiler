#include "pass/power_sum.h"
#include "pass/loop_expansion.h"
#include "pass/CFG.h"
#include "pass/loop_search.h"
#include "pass/loop_strength_reduce.h"
void power_sum_delete::run() {
    auto loop_search = new LoopSearch(m_);
    loop_search->run();
    for(auto fun: m_->get_functions()){
        if(fun->is_declaration()) continue;
        for(auto loop: loop_search->get_loop(fun)){
            // TODO: handle complex loop
            if(loop->get_loop().size() != 2) continue;
            auto entry_bb = loop->get_loop_entry();
            BasicBlock *loop_bb;
            for(auto succ: entry_bb->get_succ_basic_blocks()){
                if(loop->get_loop().find(succ) != loop->get_loop().end()){
                    loop_bb = succ;
                    break;
                }
            }
            CmpInst *cmp_instr = nullptr;
            Value *init_value = nullptr;
            Value *step = nullptr;
            Value *update_value = nullptr;
            std::set<Value *> phi_values;
            std::set<Value *> loop_vars;
            std::set<Value *> entry_vars;
            auto new_bb = BasicBlock::create(m_, "", fun);
            // find all var appeared in loop except load store call
            bool flag = true;
            for(auto instr: loop_bb->get_instructions()){
                if(!(instr->isBinary() || instr->is_br())){
                    flag = false;
                    break;
                }
            }
            if(!flag) continue;
            for(auto instr: entry_bb->get_instructions()){
                if(instr->is_cmp()){
                    cmp_instr = dynamic_cast<CmpInst *>(instr);
                    break;
                }
            }
            for(auto instr: entry_bb->get_instructions()){
                if(instr->is_phi()){
                    phi_values.insert(instr);
                }
            }
            for(auto instr: loop_bb->get_instructions()){
                if(instr->isBinary()){
                    loop_vars.insert(instr);
                }
            }
            for(auto instr: entry_bb->get_instructions()){
                if(instr->isBinary()){
                    entry_vars.insert(instr);
                }
            }
            bool find_infer = false;
            Value *infer_var = nullptr;
            Value *Upperbound = nullptr;
            std::vector<std::pair<std::string, Value *>> linear_update_queue;
            for(auto instr: entry_bb->get_instructions()){
                // find a linear increase var
                // the infer var should like var = var + const(loop invariant const)
                if(instr->is_phi()){
                    auto phi_instr = dynamic_cast<PhiInst *>(instr);
                    // analyze phi instr's two source
                    for(auto pair: phi_instr->getValueBBPair()){
                        if(!loop->contain_bb(pair.second)){
                            init_value = pair.first;
                        }
                        else{
                            update_value = pair.first;
                        }
                    }
                    // trace phi update
                    std::vector<Value *> trace_queue;
                    trace_queue.push_back(update_value);
                    linear_update_queue.clear();
                    bool success = true;
                    while(!trace_queue.empty()){
                        auto trace_instr = dynamic_cast<Instruction *>(trace_queue.back());
                        trace_queue.pop_back();
                        // an infer instr only has add or sub operation
                        if(!(trace_instr->is_add() || trace_instr->is_sub())){
                            success = false;
                            break;
                        }
                        else{
                            auto bin_instr = dynamic_cast<BinaryInst *>(trace_instr);
                            auto lhs = bin_instr->get_operand(0);
                            auto rhs = bin_instr->get_operand(1);
                            if(lhs == instr || dynamic_cast<ConstantInt *>(lhs)){
                                linear_update_queue.push_back(std::pair<std::string, Value *>("+", lhs));
                            }
                            else if(loop_vars.find(lhs) != loop_vars.end()){
                                trace_queue.push_back(lhs);
                            }
                            else{
                                success = false;
                                break;
                            }
                            if(rhs == instr || dynamic_cast<ConstantInt *>(rhs)){
                                if(bin_instr->is_add()){
                                    linear_update_queue.push_back(std::pair<std::string, Value *>("+", rhs));
                                }
                                else{
                                    linear_update_queue.push_back(std::pair<std::string, Value *>("-", rhs));
                                }
                            }
                            else if(loop_vars.find(rhs) != loop_vars.end()){
                                trace_queue.push_back(rhs);
                            }
                            else{
                                success = false;
                                break;
                            }
                        }
                    }
                    // if we find a linear increase variable, detect the relation between cmp instr
                    std::vector<Value *> work_list;
                    work_list.push_back(cmp_instr->get_operand(0));
                    work_list.push_back(cmp_instr->get_operand(1));
                    // TODO: handle more complex upperbound
                    if(cmp_instr->get_cmp_op() == CmpInst::LT && cmp_instr->get_operand(0) == instr){
//                        Upperbound = BinaryInst::create_sub(cmp_instr->get_operand(1), ConstantInt::get(1, m_), new_bb, m_);
                        Upperbound = cmp_instr->get_operand(1);
                    }
                    else if(cmp_instr->get_cmp_op() == CmpInst::LE && cmp_instr->get_operand(0) == instr){
                        Upperbound = BinaryInst::create_sub(cmp_instr->get_operand(1), ConstantInt::get(1, m_), new_bb, m_);
                    }
                    else{
                        success = false;
                    }
                    if(success){
                        while(!work_list.empty()){
                            auto val = work_list.back();
                            work_list.pop_back();
                            if(entry_vars.find(val) != entry_vars.end()){
                                auto bin_instr = dynamic_cast<BinaryInst *>(val);
                                work_list.push_back(bin_instr->get_operand(0));
                                work_list.push_back(bin_instr->get_operand(1));
                            }
                            else if(phi_values.find(val) != phi_values.end() && val != instr){
                                success = false;
                                break;
                            }
                            else{
                                continue;
                            }
                        }
                    }
                    if(success){
                        find_infer = true;
                        infer_var = instr;
                        break;
                    }
                }
            }
            if(!find_infer) continue;
            std::map<Value *, CR *> value_cr_map;
            int count = 0;
            std::cout << infer_var->get_name() << std::endl;
            for(auto pair: linear_update_queue){
                std::cout << pair.first << "  " << pair.second->get_name() << std::endl;
                if(pair.first == "+" && pair.second == infer_var) count ++;
                if(pair.first == "-" && pair.second == infer_var) count --;
            }
            if(count != 1) continue;
            std::cout << "power debug 1" << std::endl;
            for(auto pair: linear_update_queue){
                if(pair.first == "+" && pair.second != infer_var){
                    if(step){
                        step = BinaryInst::create_add(step, pair.second, new_bb, m_);
                        std::cout << step->print() << std::endl;
                    }
                    else{
                        step = pair.second;
                        std::cout << step->print() << std::endl;
                    }
                }
                else if(pair.first == "-" && pair.second != infer_var){
                    if(step){
                        step = BinaryInst::create_sub(step, pair.second, new_bb, m_);
                        std::cout << step->print() << std::endl;
                    }
                    else{
                        step = BinaryInst::create_sub(ConstantInt::get(0, m_), pair.second, new_bb, m_);
                        std::cout << step->print() << std::endl;
                    }
                }
            }
            auto loop_length = BinaryInst::create_sub(Upperbound, init_value, new_bb, m_);
            auto floor_ = BinaryInst::create_sub(step, ConstantInt::get(1, m_), new_bb, m_);
            auto upper = BinaryInst::create_add(floor_, loop_length, new_bb, m_);
            auto loop_time = BinaryInst::create_sdiv(upper, step, new_bb, m_);
            std::cout << "power debug 2" << std::endl;
            if(step == nullptr){
                new_bb->erase_from_parent();
                continue;
            }
            auto step_cr = new CR(step);
            auto base_cr = new CR(init_value, step_cr);
//            std::cout << dynamic_cast<ConstantInt *>(init_value)->get_value() << std::endl;
//            std::cout << step->print() << std::endl;
            value_cr_map[infer_var] = base_cr;
            std::set<Value *> work_list;
            for(auto phi: phi_values){
                if(phi != infer_var) work_list.insert(phi);
            }
            bool phi_success = true;
            while(!work_list.empty()){
                bool changed = true;
                // add all possible cr to value_cr_map
                while(changed){
                    changed = false;
                    for(auto instr: loop_bb->get_instructions()){
                        if(value_cr_map.find(instr) != value_cr_map.end()) continue;
                        auto add_instr = dynamic_cast<BinaryInst *>(instr);
                        if(!add_instr) continue;
                        auto lhs = add_instr->get_operand(0);
                        auto rhs = add_instr->get_operand(1);
                        CR *cr1 = nullptr;
                        CR *cr2 = nullptr;
                        if(value_cr_map.find(lhs) != value_cr_map.end()){
                            // case1: operator is contained in value_cr_map
                            cr1 = value_cr_map[lhs];
                        }
                        else if(dynamic_cast<ConstantInt *>(lhs)){
                            // case2: operator is a constant
                            auto const_val = dynamic_cast<ConstantInt *>(lhs);
                            cr1 = new CR(const_val);
                        }
                        else if(loop_vars.find(lhs) == loop_vars.end()
                        && entry_vars.find(lhs) == entry_vars.end()
                        && phi_values.find(lhs) == phi_values.end()){
                            // case3: operator is loop invariant
                            cr1 = new CR(lhs);
                        }
                        else{
                            continue;
                        }
                        if(value_cr_map.find(rhs) != value_cr_map.end()){
                            cr2 = value_cr_map[rhs];
                        }
                        else if(dynamic_cast<ConstantInt *>(rhs)){
                            auto const_val = dynamic_cast<ConstantInt *>(rhs);
                            cr2 = new CR(const_val);
                        }
                        else if(loop_vars.find(rhs) == loop_vars.end()
                        && entry_vars.find(rhs) == entry_vars.end()
                        && phi_values.find(rhs) == phi_values.end()){
                            cr2 = new CR(rhs);
                        }
                        else{
                            continue;
                        }
                        auto new_cr_ = new CR();
                        if(instr->is_add()){
                            new_cr_ = new_cr_->add(cr1, cr2, m_, new_bb);
                            value_cr_map[instr] = new_cr_;
                            changed = true;
                        }
                        else if(instr->is_sub()){
                            new_cr_ = new_cr_->sub(cr1, cr2, m_, new_bb);
                            value_cr_map[instr] = new_cr_;
                            changed = true;
                        }
                        else if(instr->is_mul()){
                            if(cr1->get_step() == nullptr && cr2->get_step() == nullptr){
                                auto new_init = BinaryInst::create_mul(cr1->get_init_val(), cr2->get_init_val(), new_bb, m_);
                                new_cr_ = new CR(new_init);
                            }
                            else if(cr1->get_step() == nullptr){
                                new_cr_ = new_cr_->mul_const(cr2, cr1->get_init_val(), m_, new_bb);
                            }
                            else if(cr2->get_step() == nullptr){
                                new_cr_ = new_cr_->mul_const(cr1, cr2->get_init_val(), m_, new_bb);
                            }
                            else{
                                new_cr_ = new_cr_->mul(cr1, cr2, m_, new_bb);
                            }
                            value_cr_map[instr] = new_cr_;
                            changed = true;
                        }
                    }
                }
                bool phi_flag = true;
                std::vector<Value *> wait_delete;
                for(auto phi: work_list){
                    auto phi_instr = dynamic_cast<PhiInst *>(phi);
                    auto trace_val = phi_instr->get_operand(2);
                    auto new_trace = new trace_node(trace_val);
                    new_trace->set_trace(new_trace);
                    std::vector<trace_node *> trace_queue;
                    trace_queue.push_back(new_trace);
                    std::vector<std::pair<std::string, Value *>> infer_queue;
                    int var_count = 0;
                    trace_node *infer_node = nullptr;
                    while(!trace_queue.empty()){
                        changed = false;
                        auto trace = trace_queue.back();
                        trace_queue.pop_back();
                        auto val = trace->use;
                        auto trace_instr = dynamic_cast<BinaryInst *>(val);
                        if(!trace_instr || trace_instr->is_mul()){
                            phi_flag = false;
                            break;
                        }
                        auto instr = dynamic_cast<BinaryInst *>(val);
                        auto lhs = instr->get_operand(0);
                        auto rhs = instr->get_operand(1);
                        if(value_cr_map.find(lhs) != value_cr_map.end()){
                            infer_queue.push_back(std::pair<std::string, Value *>("+", lhs));
                        }
                        else if(lhs == phi){
                            auto x = new trace_node(lhs);
                            x->set_trace(trace);
                            infer_node = x;
                            var_count ++;
                        }
                        else if(dynamic_cast<ConstantInt *>(lhs)){
                            auto const_lhs = dynamic_cast<ConstantInt *>(lhs);
                            infer_queue.push_back(std::pair<std::string, Value *>("+", lhs));
                        }
                        else if(loop_vars.find(lhs) == loop_vars.end()
                        && entry_vars.find(lhs) == entry_vars.end()
                        && phi_values.find(lhs) == phi_values.end()){
                            infer_queue.push_back(std::pair<std::string, Value *>("+", lhs));
                        }
                        else if(loop_vars.find(lhs) != loop_vars.end()){
                            auto x = new trace_node(lhs);
                            x->set_trace(trace);
                            trace_queue.push_back(x);
                        }
                        else{
                            phi_flag = false;
                            break;
                        }
                        if(value_cr_map.find(rhs) != value_cr_map.end()){
                            if(trace_instr->is_add()){
                                infer_queue.push_back(std::pair<std::string, Value *>("+", rhs));
                            }
                            else{
                                infer_queue.push_back(std::pair<std::string, Value *>("-", rhs));
                            }
                        }
                        else if(rhs == phi){
                            auto x = new trace_node(rhs);
                            x->set_trace(trace);
                            infer_node = x;
                            count ++;
                        }
                        else if(dynamic_cast<ConstantInt *>(rhs)){
                            auto const_lhs = dynamic_cast<ConstantInt *>(rhs);
                            if(trace_instr->is_add()){
                                infer_queue.push_back(std::pair<std::string, Value *>("+", rhs));
                            }
                            else{
                                infer_queue.push_back(std::pair<std::string, Value *>("-", rhs));
                            }
                        }
                        else if(loop_vars.find(rhs) == loop_vars.end()
                        && entry_vars.find(rhs) == entry_vars.end()
                        && phi_values.find(rhs) == phi_values.end()){
                            if(trace_instr->is_add()){
                                infer_queue.push_back(std::pair<std::string, Value *>("+", rhs));
                            }
                            else{
                                infer_queue.push_back(std::pair<std::string, Value *>("-", rhs));
                            }
                        }
                        else if(loop_vars.find(rhs) != loop_vars.end()){
                            auto x = new trace_node(rhs);
                            x->set_trace(trace);
                            trace_queue.push_back(x);
                        }
                        else {
                            phi_flag = false;
                            break;
                        }
                    }
                    if(!phi_flag){
                        phi_success = false;
                        continue;
                    }
                    else{
                        CR *new_cr = new CR(ConstantInt::get(0, m_));
                        for(auto pair: infer_queue){
                            if(pair.first == "+"){
                                if(value_cr_map.find(pair.second) != value_cr_map.end()){
                                    new_cr = new_cr->add(new_cr, value_cr_map[pair.second], m_, new_bb);
                                }
                                else{
                                    auto temp_cr = new CR(pair.second);
                                    new_cr = new_cr->add(new_cr, temp_cr, m_, new_bb);
                                }
                            }
                            else{
                                if(value_cr_map.find(pair.second) != value_cr_map.end()){
                                    new_cr = new_cr->sub(new_cr, value_cr_map[pair.second], m_, new_bb);
                                }
                                else{
                                    auto temp_cr = new CR(pair.second);
                                    new_cr = new_cr->sub(new_cr, temp_cr, m_, new_bb);
                                }
                            }
                        }
                        CR *phi_cr = new CR(phi_instr->get_operand(0), new_cr);
                        value_cr_map[phi] = phi_cr;
                        wait_delete.push_back(phi);
                        std::cout << 11 << std::endl;
                    }
                }
                for(auto x: wait_delete){
                    work_list.erase(x);
                }
                if(!phi_success) break;
            }

            // for debug
            for(auto x: value_cr_map){
                std::cout << x.first->get_name() << std::endl;
                auto p = x.second;
                while(p){
                    if(dynamic_cast<ConstantInt *>(p->get_init_val())){
                        std::cout << dynamic_cast<ConstantInt *>(p->get_init_val())->get_value() << std::endl;
                    }
                    else{
                        std::cout << p->get_init_val()->print() << std::endl;
                    }
                    p = p->get_step();
                }
            }

            if(!phi_success){
                new_bb->erase_from_parent();
            }
            else{
                std::cout << "uncss" << std::endl;
                for(auto phi: phi_values){
                    std::cout << phi->get_name() << std::endl;
                    if(value_cr_map.find(phi) != value_cr_map.end()){
                        auto x = value_cr_map[phi];
                        std::cout << "gen code" << std::endl;
                        auto new_val = x->gen_code_for_cr(loop_time, new_bb, m_);
                        phi->replace_all_use_with(new_val);
                    }
                }
                std::cout << "//////////////" << std::endl;
                std::cout << infer_var->get_name() << std::endl;
                // change bb relation
                // outer ---> entry --> new_bb --> exit
                auto target_bb = dynamic_cast<BasicBlock *>(entry_bb->get_terminator()->get_operand(2));
                loop_bb->erase_from_parent();
                std::vector<Instruction *> wait_delete_instr;
                for(auto instr: entry_bb->get_instructions()){
                    wait_delete_instr.push_back(instr);
                }
                for(auto instr: wait_delete_instr){
                    entry_bb->delete_instr(instr);
                }
                entry_bb->remove_succ_basic_block(target_bb);
                auto entry_to_new_bb = BranchInst::create_br(new_bb, entry_bb);
                auto new_bb_to_target = BranchInst::create_br(target_bb, new_bb);
            }
        }
    }
}