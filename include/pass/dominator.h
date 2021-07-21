#ifndef SYSYC_DOMINATORS_HPP
#define SYSYC_DOMINATORS_HPP

#include"pass.h"
#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include <list>
#include <set>
#include <map>

class dominator : public Pass{
public:
    explicit dominator(Module *m) : Pass(m){}
    ~dominator(){};
    void run() override;

    BasicBlock *get_immediate_dominance(BasicBlock *bb) { return immediate_dominance[bb]; }
    void create_immediate_dominance(Function *f);
    void set_immediate_dominance(BasicBlock *bb, BasicBlock *idom) { immediate_dominance[bb] = idom; }

    void create_dominance_frontier(Function *f);
    void create_reverse_post_order(Function *f);
    void post_order_visit(BasicBlock *bb, std::set<BasicBlock *> &visited);
    BasicBlock *intersect(BasicBlock *b1, BasicBlock *b2);
    std::set<BasicBlock *> &get_dominance_frontier(BasicBlock *bb) { return dominannce_frontier[bb]; }
    void add_dom_tree_succ_block(BasicBlock *bb, BasicBlock *dom_tree_succ_bb) { dom_tree_succ_blocks[bb].insert(dom_tree_succ_bb); }
    std::set<BasicBlock *> get_dom_tree_succ_blocks(BasicBlock *bb) { return dom_tree_succ_blocks[bb]; }
    void add_dominance_frontier(BasicBlock *bb, BasicBlock *dom_frontier_bb) { dominannce_frontier[bb].insert(dom_frontier_bb); }
    void create_dom_tree_succ(Function *f);
    void print_dom_tree();
private:

    std::list<BasicBlock *> reverse_post_order_;
    std::map<BasicBlock *, int> post_order_id_;   // the root has highest ID

    std::map<BasicBlock *, BasicBlock *>immediate_dominance ;    // immediate dominance
    std::map<BasicBlock *, std::set<BasicBlock *>> dominannce_frontier;    // dominance frontier set
    std::map<BasicBlock *, std::set<BasicBlock *>> dom_tree_succ_blocks;
};

#endif