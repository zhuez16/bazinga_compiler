#ifndef SYSYC_DOMINATORS_HPP
#define SYSYC_DOMINATORS_HPP

#include"pass_manager.h"
#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include <list>
#include <set>
#include <map>

class dominator : public Pass {
public:
    explicit dominator(Module *m) : Pass(m) {}

    ~dominator() = default;

    void run() override;

    BasicBlock *getImmediateDominance(BasicBlock *bb) { return _iDomMap[bb]; }

    std::set<BasicBlock *> getDomTreeSuccessorBlocks(BasicBlock *bb) { return _successorMap[bb]; }

    std::set<BasicBlock *> &getDominanceFrontier(BasicBlock *bb) { return _frontierMap[bb]; }

    BasicBlock *intersect(BasicBlock *b1, BasicBlock *b2);

    std::list<BasicBlock *> getRPOQueue(Function *f) {
        create_reverse_post_order(f);
        return _reversedPostOrderQueue;
    }

    /**
     * If child is dominate by parent then return true else false
     * @param child
     * @param parent
     * @return
     */
    bool isDominatedBy(BasicBlock *child, BasicBlock *parent) {
        auto sets = getDomTreeSuccessorBlocks(parent);
        return sets.find(child) != sets.end();
    }

private:
    void create_immediate_dominance(Function *f);

    void set_immediate_dominance(BasicBlock *bb, BasicBlock *imm_dom) { _iDomMap[bb] = imm_dom; }

    void create_dominance_frontier(Function *f);

    void create_reverse_post_order(Function *f);

    void dfs(BasicBlock *bb, std::set<BasicBlock *> &visited, int &id);

    void add_dom_tree_succ_block(BasicBlock *bb, BasicBlock *dom_tree_succ_bb) {
        _successorMap[bb].insert(dom_tree_succ_bb);
    }

    void add_dominance_frontier(BasicBlock *bb, BasicBlock *dom_frontier_bb) {
        _frontierMap[bb].insert(dom_frontier_bb);
    }

    void create_dom_tree_succ(Function *f);

    void print_dom_tree();

private:

    std::list<BasicBlock *> _reversedPostOrderQueue;
    std::map<BasicBlock *, int> _postOrderID;

    std::map<BasicBlock *, BasicBlock *> _iDomMap;
    std::map<BasicBlock *, std::set<BasicBlock *>> _frontierMap;
    std::map<BasicBlock *, std::set<BasicBlock *>> _successorMap;
};

#endif