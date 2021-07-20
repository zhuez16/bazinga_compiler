//
// Created by mskhana on 2021/7/20.
//

#ifndef BAZINGA_COMPILER_ACTIVE_VARS_H
#define BAZINGA_COMPILER_ACTIVE_VARS_H
#include "pass_manager.h"
#include "IR/Constant.h"
#include "IR/Instruction.h"
#include "IR/Module.h"
#include "IR/Value.h"
#include "IR/IRbuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>

class active_vars: public Pass {
public:
    active_vars(Module *m): Pass(m){}
    void run();
private:
    Function *func_;
    std::map<BasicBlock*,std::unordered_set<Value *> > active, live_in, live_out;
    std::map<BasicBlock*,std::unordered_set<Value *> > use, def, phi_op;
    std::map<BasicBlock*,std::set<std::pair<BasicBlock *,Value  *>>> phi_op_pair;

};


#endif //BAZINGA_COMPILER_ACTIVE_VARS_H
