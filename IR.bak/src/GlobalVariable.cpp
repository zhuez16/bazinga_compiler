#include "IR/GlobalVariable.h"

#include <utility>
GlobalVariable::GlobalVariable(const std::string& name, Module *m, Type *type, bool is_const, Constant *init){
    m->add_global_var(this);
}
Constant* GlobalVariable::get_init(){
    return this->init_val_;
}

GlobalVariable* GlobalVariable::create(const std::string& name, Module *m, Type *type, bool is_const, Constant *init){
    return new GlobalVariable(name, m, type, is_const, init);
}