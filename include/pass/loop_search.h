
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
    std::vector<Loop *> work_list;
    std::map<BasicBlock *, int> DFN;
    std::map<BasicBlock *, int> Low;
    std::stack<BasicBlock *> loop_stack;
    std::set<BasicBlock *> in_stack;
    std::set<BasicBlock *> visited;
    std::set<std::pair<BasicBlock *, BasicBlock *>> edges;
    std::map<Loop *, std::set<Loop *>> child_loop;
    std::map<Function *, std::map<BasicBlock *, Loop *>> _nested_loop;
    int index = 0;
    Function *cur_fun;
    Loop *get_inner_loop(const std::set<Loop *>& l, BasicBlock *bb);
    void build_pre_succ_relation(Loop *loop);
    void build_map();
public:
    void print_loop();
    LoopSearch(Module *m) : Pass(m){}
    ~LoopSearch(){};
    std::set<Loop *> get_child_loop(Loop *loop) { return child_loop[loop]; }
    std::set<Loop *> get_loop(Function *fun) { return fun_loop[fun]; }

    /**
     * 获取包含该基本块的最小循环
     * @param f
     * @param bb
     * @return
     */
    Loop *get_smallest_loop(Function *f, BasicBlock *bb) { return _nested_loop[f][bb]; }
    Loop *get_smallest_loop(BasicBlock *bb) { return _nested_loop[bb->get_parent()][bb]; }
    void Tarjan(BasicBlock *bb, std::set<BasicBlock *> blocks);
    void run() override;
};

class Loop{
private:
    BasicBlock *entry_block;
    std::map<BasicBlock *, std::set<BasicBlock *>> loop_pre;
    std::map<BasicBlock *, std::set<BasicBlock *>> loop_succ;
    std::set<BasicBlock *> loop_block;
public:
    std::set<BasicBlock *> get_loop() { return loop_block; };
    void add_loop_block(BasicBlock *bb) { loop_block.insert(bb); }
    std::set<BasicBlock *> get_loop_bb_pre(BasicBlock *bb) { return loop_pre[bb]; }
    void add_loop_bb_pre(BasicBlock *bb, BasicBlock *pre) { loop_pre[bb].insert(pre); }
    void add_loop_bb_succ(BasicBlock *bb, BasicBlock *succ) { loop_succ[bb].insert(succ); }
    std::set<BasicBlock *> get_loop_bb_succ(BasicBlock *bb) { return loop_succ[bb]; }
    BasicBlock *get_loop_entry() { return entry_block; }
    void set_entry_block(BasicBlock *entry) { this->entry_block = entry; }
    bool contain_bb(BasicBlock *bb) { return loop_block.find(bb) != loop_block.end(); }
};


#endif