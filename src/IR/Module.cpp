#include <Module.h>
#include <Type.h>
#include <Function.h>
#include <GlobalVariable.h>
Module::Module(){
    // todo
}

void Module::add_function(Function* func){
    this->function_lists.push_back(func);
}

void Module::add_global_var(GlobalVariable* var){
    this->global_vars.push_back(var);
}

std::list<GlobalVariable *> Module::get_global_variable(){
    return this->global_vars;
}

std::string Module::get_instr_name(Instruction::OpID oprand){
    
}

Type* Module::get_void_type(){
    return this->VoidType;
}

Type* Module::get_int_type(){
    return this->IntType;
}

Type* Module::get_bool_type(){
    return this->BoolType;
}

void Module::set_print_name(){
    // todo
}