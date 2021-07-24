
#ifndef SYSYC_GLO2LOC_H
#define SYSYC_GLO2LOC_H

#include "IR/Module.h"
#include "IR/Function.h"
#include "IR/IRbuilder.h"
#include "IR/BasicBlock.h"
#include "IR/Instruction.h"
#include "pass.h"

class Global2Local : public Pass{
private:
    Function * func_;
    std::set<Function *> called_fun;
    std::map<Function *, std::set<Value *>> fun_use_var;
    std::map<Value *, std::set<Function *>> var_used_by_fun;
    std::set<GlobalVariable *> used_global_var;
    void create_fun_use_var();
    void print_fun_use_var();
public:
    Global2Local(Module *m) : Pass(m){}
    ~Global2Local(){};
    void create_func_call_tree();
    void run() override;
};

#endif