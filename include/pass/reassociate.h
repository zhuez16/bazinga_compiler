//
// Created by mskhana on 2021/8/1.
//

#ifndef BAZINGA_COMPILER_REASSOCIATE_H
#define BAZINGA_COMPILER_REASSOCIATE_H

#include "pass_manager.h"
#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include <list>
#include <set>
#include <map>

class reassociate: public Pass {
public:
    explicit reassociate(Module *m) : Pass(m) {}

    ~reassociate() = default;

    void run() override;

    bool isUnmovableInstr(Instruction *instr){
        if (instr->is_alloca()
         || instr->is_phi()
         || instr->is_load()
         || instr->is_call()
         || instr->is_div()
         || instr->is_rem())
            return true;
        return false;
    }

    bool getInstrRank(BasicBlock *bb);

    bool get_reassociable(Value *val){
        if (val->get_use_list().size()<=1)
            return true;
        return false;
    }

private:
    std::map<Instruction *, int> inst_rank;
};


#endif //BAZINGA_COMPILER_REASSOCIATE_H
