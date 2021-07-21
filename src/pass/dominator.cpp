#include "pass/dominator.h"
#include "queue"
#include "iostream"
// reference : A Simple, Fast Dominance Algorithm
void dominator::run()
{
    for (auto f : m_->get_functions()) {
        if (f->get_basic_blocks().size() == 0)
            continue;
        for (auto bb : f->get_basic_blocks() )
        {
            immediate_dominance.insert({bb ,{}});
            dominannce_frontier.insert({bb ,{}});
            dom_tree_succ_blocks.insert({bb ,{}});
        }

        create_reverse_post_order(f);
        create_immediate_dominance(f);
        create_dominance_frontier(f);
        create_dom_tree_succ(f);
    }
}

void dominator::create_reverse_post_order(Function *f)
{
    reverse_post_order_.clear();
    post_order_id_.clear();
    std::set<BasicBlock *> visited;
    post_order_visit(f->get_entry_block(), visited);
    reverse_post_order_.reverse();
}

void dominator::post_order_visit(BasicBlock *bb, std::set<BasicBlock *> &visited)
{
    visited.insert(bb);
    for (auto b : bb->get_succ_basic_blocks()) {
        if (visited.find(b) == visited.end())
            post_order_visit(b, visited);
    }
    post_order_id_[bb] = reverse_post_order_.size();
    reverse_post_order_.push_back(bb);
}

void dominator::create_immediate_dominance(Function *f){
    // init
    for (auto bb : f->get_basic_blocks())
        set_immediate_dominance(bb, nullptr);
    auto root = f->get_entry_block();
    set_immediate_dominance(root, root);

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto bb : this->reverse_post_order_) {
            if (bb == root) {
                continue;
            }

            // find one pred which has idom
            BasicBlock *pred = nullptr;
            for (auto p : bb->get_pre_basic_blocks()) {
                if (get_immediate_dominance(p)) {
                    pred = p;
                    break;
                }
            }
            assert(pred);

            BasicBlock *new_idom = pred;
            for (auto p : bb->get_pre_basic_blocks()) {
                if (p == pred)
                    continue;
                if (get_immediate_dominance(p)) {
                    new_idom = intersect(p, new_idom);
                }
            }
            if (get_immediate_dominance(bb) != new_idom) {
                set_immediate_dominance(bb, new_idom);
                changed = true;
            }
        }
    }

}

// find closest parent of b1 and b2
BasicBlock *dominator::intersect(BasicBlock *b1, BasicBlock *b2)
{
    while (b1 != b2) {
        while (post_order_id_[b1] < post_order_id_[b2]) {
            assert(get_immediate_dominance(b1));
            b1 = get_immediate_dominance(b1);
        }
        while (post_order_id_[b2] < post_order_id_[b1]) {
            assert(get_immediate_dominance(b2));
            b2 = get_immediate_dominance(b2);
        }
    }
    return b1;
}

void dominator::create_dominance_frontier(Function *f)
{
    for (auto bb : f->get_basic_blocks()) {
        if (bb->get_pre_basic_blocks().size() >= 2) {
            for (auto p : bb->get_pre_basic_blocks()) {
                auto runner = p;
                while (runner != get_immediate_dominance(bb)) {
                    add_dominance_frontier(runner, bb);
                    runner = get_immediate_dominance(runner);
                }
            }
        }
    }
}

void dominator::create_dom_tree_succ(Function *f)
{
    for (auto bb : f->get_basic_blocks()) {
        auto idom = get_immediate_dominance(bb);
        // e.g, entry bb
        if (idom != bb) {
            add_dom_tree_succ_block(idom, bb);
        }
    }
}