#include <Module.h>
#include <Type.h>
#include <Function.h>
#include <GlobalVariable.h>
#include <string>

Module::Module(std::string name){
    this->VoidType = new Type(Type::void_type);
    this->IntType = new Type(Type::Integer_type);
    this->BoolType = new IntergetType(1);
    this->LabelType = new Type(Type::Label_type);
    this->Int32PtrType = new PointerType(this->IntType);
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

Type* Module::get_Void_type(){
    return this->VoidType;
}

// Type* Module::get_Int_type(){
//     return this->IntType;
// }

// Type* Module::get_bool_type(){
//     return this->BoolType;
// }

Type* Module::get_Label_type(){
    return this->LabelType;
}

IntegerType* Module::get_Int1_type(){
    return this->BoolType;
}

IntegerType* Module::get_Int32_type(){
    return this->IntType;
}

PointerType* Module::get_Int32Ptr_type(){
    return this->Int32PtrType;
}

void Module::set_print_name(){
    // todo
}