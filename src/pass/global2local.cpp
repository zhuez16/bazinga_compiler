#include "pass/global2local.h"
#include "map"

std::map<Function *, std::set<Function *>> func_call_tree;
std::set<Function *> recursive_fun;
void Global2Local::create_func_call_tree() {
    for(auto func: m_->get_functions()){
        for(auto bb: func->get_basic_blocks()){
            for(auto instr: bb->get_instructions()){
                if(instr->is_call()){
                    func_call_tree[func].insert(dynamic_cast<Function *>(instr->get_operand(0)));
                    called_fun.insert(dynamic_cast<Function *>(instr->get_operand(0)));
                    if(func == dynamic_cast<Function *>(instr->get_operand(0)) ){
                        recursive_fun.insert(func);
//                        std::cout<<func->get_name() <<std::endl;
//                        std::cout<<dynamic_cast<Function *>(instr->get_operand(0))->get_name() <<std::endl;
                    }
                }
            }
        }
    }
}

void Global2Local::create_fun_use_var() {
    for(auto func: m_->get_functions()){
        for(auto bb: func->get_basic_blocks()){
            for(auto instr: bb->get_instructions()){
                if(instr->is_store()){
//                    std::cout<<"is store"<<std::endl;
                    auto store_instr = dynamic_cast<StoreInst *>(instr);
                    auto l_val = store_instr->get_lval();
//                    std::cout<<l_val->get_name()<<std::endl;
                    if(dynamic_cast<GlobalVariable *>(l_val)){
                        fun_use_var[func].insert(l_val);
                        var_used_by_fun[l_val].insert((func));
                        used_global_var.insert(dynamic_cast<GlobalVariable *>(l_val));
                    }
                }
                else if(instr->is_load()){
//                    std::cout<<"is load"<<std::endl;
                    auto load_instr = dynamic_cast<LoadInst *>(instr);
                    auto l_val = load_instr->get_lval();
//                    std::cout<<l_val->get_name()<<std::endl;
                    if(dynamic_cast<GlobalVariable *>(l_val)){
                        fun_use_var[func].insert(l_val);
                        var_used_by_fun[l_val].insert((func));
                        used_global_var.insert(dynamic_cast<GlobalVariable *>(l_val));
                    }
                }
                else if(instr->is_gep()){
//                    std::cout<<"is gep"<<std::endl;
                    auto gep_instr = dynamic_cast<GetElementPtrInst *>(instr);
                    auto l_val = gep_instr->get_operand(0);
//                    std::cout<<l_val->get_name()<<std::endl;
                    if(dynamic_cast<GlobalVariable *>(l_val)){
                        fun_use_var[func].insert(l_val);
                        var_used_by_fun[l_val].insert((func));
                        used_global_var.insert(dynamic_cast<GlobalVariable *>(l_val));
                    }
                }
            }
        }
    }
}

void print_fun_call_tree(){
    for(auto x: func_call_tree){
        for(auto y: x.second){
            std::cout<< x.first->get_name() << " call " << y->get_name() << std::endl;
        }
    }
}

void Global2Local::print_fun_use_var(){
    for(auto x: fun_use_var){
        for(auto y: x.second){
            std::cout<< x.first->get_name() << " use " << y->get_name() << std::endl;
        }
    }
}

void Global2Local::run() {
    create_func_call_tree();
//    print_fun_call_tree();
    create_fun_use_var();
//    print_fun_use_var();
    for(auto global_var: m_->get_global_variable()){
        if(used_global_var.find(global_var) == used_global_var.end()){
            m_->delete_global_variable(global_var);
        }
        else if(var_used_by_fun[global_var].size() == 1 ){
            auto use_fun = *(var_used_by_fun[global_var].begin());
            if(recursive_fun.find(use_fun) != recursive_fun.end()) continue;
            if(called_fun.find(use_fun) == called_fun.end()){
                // if func_ is not called by any function
                if(global_var->get_init()->get_type()->is_int32_type()){
                    auto init_val = global_var->get_init();
                    auto entry_block = use_fun->get_entry_block();
                    std::vector<Instruction *> temp_inst_store;
                    for(auto instr: entry_block->get_instructions()){
                        temp_inst_store.push_back(instr);
                    }
                    entry_block->get_instructions().clear();
                    auto alloca_for_global = AllocaInst::create_alloca(m_->get_int32_type(), entry_block);
                    auto store_for_global = StoreInst::create_store(init_val, alloca_for_global, entry_block);
                    alloca_for_global->set_parent(entry_block);
                    store_for_global->set_parent(entry_block);
                    for(auto instr: temp_inst_store){
                        entry_block->add_instruction(instr);
                    }
                    global_var->replace_all_use_with(alloca_for_global);
                    m_->delete_global_variable(global_var);
                }
                else{
                    // TODO: is it necessary to localization an global array?
                }
            }
            else if(func_call_tree[func_].empty()){
                // if func_ does not call any other function
                if(global_var->get_init()->get_type()->is_int32_type()){
                    if(use_fun->get_basic_blocks().size() != 1) continue;
                    auto entry_block = use_fun->get_entry_block();
                    std::vector<Instruction *> temp_inst_store;
                    for(auto instr: entry_block->get_instructions()){
                        temp_inst_store.push_back(instr);
                    }
                    Value *cur_global_value = nullptr;
                    Value *final_store = nullptr;
                    std::vector<Instruction *> wait_delete;
                    // detect the last assignment
                    for(auto instr: entry_block->get_instructions()){
                        if(instr->is_store()){
                            auto store_instr = dynamic_cast<StoreInst *>(instr);
                            auto l_val = store_instr->get_lval();
                            auto l_val_global = dynamic_cast<GlobalVariable *>(l_val);
                            if(l_val_global && l_val_global == global_var){
                                cur_global_value = store_instr->get_rval();
                                final_store = instr;
                                wait_delete.push_back(instr);
                            }
                        }
                        if(instr->is_load()){
                            auto load_instr = dynamic_cast<LoadInst *>(instr);
                            auto r_val = load_instr->get_operand(0);
                            auto r_val_global = dynamic_cast<GlobalVariable *>(r_val);
                            if(r_val_global && r_val_global == global_var && cur_global_value != nullptr){
                                instr->replace_all_use_with(cur_global_value);
                                wait_delete.push_back(instr);
                            }
                        }
                    }
                    for(auto instr: wait_delete){
                        if(instr == final_store) continue;
                        entry_block->delete_instr(instr);
                    }
//                    entry_block->get_instructions().clear();
//                    auto alloca_for_global = AllocaInst::create_alloca(m_->get_int32_type(), entry_block);
//                    auto global_load = LoadInst::create_load(global_var->get_type()->get_pointer_element_type(), global_var, entry_block);
//                    for(auto instr: temp_inst_store){
//                        entry_block->add_instruction(instr);
//                    }
//                    global_var->replace_all_use_with(global_load);
//                    global_load->set_operand(0, global_var);
//                    std::cout << global_load->get_operand(0)->get_type()->is_pointer_type() << std::endl;
//                    if(final_store != nullptr){
//                        auto store_for_global = StoreInst::create_store(final_store, global_var, entry_block);
//                    }
                }
                else{
                    // TODO: is it necessary to localization an global array?
                }
            }
        }
    }
}

