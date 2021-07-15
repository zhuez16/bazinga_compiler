#include <GlobalVariable.h>
#include <Constant.h>
#include <Module.h>
GlobalVariable::GlobalVariable(std::string name, Module *m, Type *type, bool is_const, Constant *init){
    m->add_global_var(this);
}
Constant* GlobalVariable::get_init(){
    return this->init_val_;
}

GlobalVariable* GlobalVariable::create(std::string name, Module *m, Type *type, bool is_const, Constant *init){
    return new GlobalVariable(name, m, type, is_const, init);
}