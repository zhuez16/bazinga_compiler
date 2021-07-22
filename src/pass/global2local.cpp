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
                    if(func == dynamic_cast<Function *>(instr->get_operand(0)) ){
                        recursive_fun.insert(func);
                        std::cout<<func->get_name() <<std::endl;
                        std::cout<<dynamic_cast<Function *>(instr->get_operand(0))->get_name() <<std::endl;
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
    }
}

