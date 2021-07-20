#include "Pass/dominator.h"
#include "queue"
#include "iostream"
// reference : A Simple, Fast Dominance Algorithm
void dominator::run() {
    for (auto f : m_->get_functions()) {
        if(f->get_basic_blocks().size() >= 1){
            printf("dom debug 1\n");
            create_immediate_dominance(f);
            printf("dom debug 2\n");
            create_dominance_frontier(f);
            printf("dom debug 3\n");
        }
    }
}

void dominator::create_immediate_dominance(Function *f) {
    // TODO:change into bettor algorithm
    auto entry_block = f->get_entry_block();
    for(auto bb: f->get_basic_blocks()){
        std::cout <<f->get_basic_blocks().size() << std::endl;
        if(bb->get_pre_basic_blocks().size() == 0){
            set_immediate_dominator(bb, bb);//root block has no one to dom it
        }
        else if(bb->get_pre_basic_blocks().size() == 1){
            // only one pre block
            BasicBlock* new_bb = bb->get_pre_basic_blocks().front();
            set_immediate_dominator(bb, new_bb);
            std::cout << "in set idom" << std::endl;
            std::cout << bb->get_name() << std::endl;
            std::cout << new_bb->get_name() << std::endl;
        }
        else{
            // has multi pre block
            // run bfs to find dom block
            std::queue<BasicBlock *> block_queue;
            std::set<BasicBlock *> visited;
            block_queue.push(bb);
            while(!block_queue.empty()){
                auto front = block_queue.front();
                block_queue.pop();
                if(visited.find(front) != visited.end()){
                    set_immediate_dominator(bb, front);
                    std::cout << "in set idom" << std::endl;
                    std::cout << bb->get_name() << std::endl;
                    std::cout << front->get_name() << std::endl;
                    break;
                }
                else{
                    visited.insert(front);
                    for(auto pre_bb: front->get_pre_basic_blocks()){
                        block_queue.push(pre_bb);
                    }
                }
            }
        }
    }
}

void dominator::create_dominance_frontier(Function *f) {
    for(auto bb: f->get_basic_blocks()){
        if(bb->get_pre_basic_blocks().size() >= 2){
            for(auto pre: bb->get_pre_basic_blocks()){
                auto runner = pre;
                std::cout << bb->get_name() << std::endl;
                std::cout << runner->get_name() << std::endl;
                std::cout << get_immediate_dominator(bb)->get_name() << std::endl;
                while(runner != get_immediate_dominator(bb)){
                    add_dominance_frontier(runner, bb);
                    runner = get_immediate_dominator(runner);
                }
            }
        }
    }
}