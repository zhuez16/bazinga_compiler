
#ifndef SYSYC_LOOPSEARCH_H
#define SYSYC_LOOPSEARCH_H

#include "IR/Module.h"
#include "IR/Function.h"
#include "IR/IRbuilder.h"
#include "IR/BasicBlock.h"
#include "IR/Instruction.h"
#include "pass.h"
#include "stack"

class Loop;
class LoopBlock;

class LoopSearch : public Pass{
private:
    std::map<Function *, std::set<Loop *>> fun_loop;
    std::map<BasicBlock *, int> DFN;
    std::map<BasicBlock *, int> Low;
    std::stack<BasicBlock *> loop_stack;
    std::set<BasicBlock *> in_stack;
    std::set<BasicBlock *> visited;
    std::set<std::pair<BasicBlock *, BasicBlock *>> edges;
    int index = 0;
    Function *cur_fun;
public:
    void print_loop();
    LoopSearch(Module *m) : Pass(m){}
    ~LoopSearch(){};
    void Tarjan(BasicBlock *);
    void run() override;
};

class Loop{
private:
    BasicBlock *entry_block;
    std::set<BasicBlock *> loop_block;
public:
    std::set<BasicBlock *> get_loop() { return loop_block; };
    void add_loop_block(BasicBlock *bb) { loop_block.insert(bb); }
    BasicBlock *get_loop_entry() { return entry_block; }
    void set_entry_block(BasicBlock *entry) { this->entry_block = entry; }
};

class LoopBlock : public BasicBlock{
private:
    std::set<LoopBlock *> loop_pre;
    std::set<LoopBlock *> loop_succ;
public:
    std::set<LoopBlock *> get_loop_pre_block() { return this->loop_pre; }
    std::set<LoopBlock *> get_loop_succ_block() { return this->loop_succ; }
    void add_loop_pre_block(LoopBlock *pre) { this->loop_pre.insert(pre); }
    void add_loop_succ_block(LoopBlock *succ) { this->loop_succ.insert(succ); }
    void delete_loop_pre_block(LoopBlock *pre) { this->loop_pre.erase(pre); }
    void delete_loop_succ_block(LoopBlock *succ) { this->loop_succ.erase(succ); }
};

#endif