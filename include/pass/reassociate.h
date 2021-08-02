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
/*
 * Try to build a simplest reassociate pass
 * Reassociate all add expr
 * First transform all expr like A+(B+C) to (B+C)+A
 * And expr like (A+B)+(C+D) to ((A+B)+C)+D
 * then try to bubble up all the constant
 * merge all the constant in the succ of the expr.
 * */

struct ValueEntry{
    int rank;
    Value *val;
    ValueEntry(int rank, Value *val):rank(rank),val(val){}
};
inline bool operator <(const ValueEntry &l, const ValueEntry &r){
    return l.rank > r.rank;
}

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


    bool isReassociable(Value *val){
        if (val->get_use_list().size()<=1)
            return true;
        return false;
    }
    void moveInstrTo(Instruction *from, Instruction *to){
        assert(from->get_use_list().size() <= 1);
        auto remove_bb=from->get_parent();
        remove_bb->delete_instr(from);


        auto *insertbb=to->get_parent();
        for (auto it:insertbb->get_instructions()){
            if (it->getSuccInst()==to){

            }
        }
    }
    void fetchAllInstructions(Function *func);

    void BuildRankMap(Function *func);
    int getValueRank(Value *val);
    void ReassociateExpr(BinaryInst *instr);
    void RewriteExprTree(BinaryInst *instr, std::vector<ValueEntry> &expr);
    Value *OptimizeExpression(BinaryInst *instr, std::vector<ValueEntry> &expr);
    void linearExprTree(BinaryInst *instr, std::vector<ValueEntry> &expr);
    void linearExpr(BinaryInst *instr);
private:
    std::map<BasicBlock *, int> rank_map;
    std::map<Value *, int> value_rank_map;
};


#endif //BAZINGA_COMPILER_REASSOCIATE_H
