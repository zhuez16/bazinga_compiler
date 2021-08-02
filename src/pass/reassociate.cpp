//
// Created by mskhana on 2021/8/1.
//

#include "include/pass/reassociate.h"


//use rank to get all the instructions that can be reassociated.
void reassociate::BuildRankMap(Function *func) {
    int rank=2;
    for (auto *arg:func->get_args()){
        auto val=static_cast<Value *> (arg);
        this->value_rank_map[val]=++rank;
    }
    for (auto *bb:func->get_basic_blocks()){
        int bbrank=this->rank_map[bb]=++rank<<10;
        for (auto *instr:bb->get_instructions()){
            if (isUnmovableInstr(instr)){
                this->value_rank_map[instr]=++bbrank;
            }
        }
    }
}

int reassociate::getValueRank(Value *val) {
    auto *instr=dynamic_cast<Instruction *> (val);
    if (instr==nullptr) return 0;
    if (this->value_rank_map[val])
        return this->value_rank_map.at(val);
    for (auto op: instr->get_operands()){
        this->value_rank_map[val]=std::max(this->value_rank_map[val],this->value_rank_map[op]);
    }
    if (!instr->get_type()->is_integer_type()) this->value_rank_map[val]++;
    return this->value_rank_map[val];
}

void reassociate::ReassociateExpr(BinaryInst *instr) {

}

void reassociate::RewriteExprTree(BinaryInst *instr, std::vector<ValueEntry> &expr) {

}

Value *reassociate::OptimizeExpression(BinaryInst *instr, std::vector<ValueEntry> &expr) {
    return nullptr;
}

void reassociate::linearExprTree(BinaryInst *instr, std::vector<ValueEntry> &expr) {

}
void reassociate::linearExpr(BinaryInst *instr) {
    BinaryInst *left=dynamic_cast<BinaryInst *> (instr->get_operand(0));
    BinaryInst *right=dynamic_cast<BinaryInst *> (instr->get_operand(1));
//TODO make expr like A+(B+C) to (B+C)+A

}
