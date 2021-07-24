#include "pass/loop_search.h"

void LoopSearch::run() {
    for(auto fun: m_->get_functions()){
        if(fun->get_basic_blocks().size() == 0) continue;
        std::set<BasicBlock *> visit;
        std::vector<BasicBlock *> work_list;
        work_list.push_back(fun->get_entry_block());
        while(!work_list.empty()){
            auto bb = work_list.back();
            work_list.pop_back();
            for(auto succ_bb: bb->get_succ_basic_blocks()){
                edges.insert(std::pair<BasicBlock *, BasicBlock *>(bb, succ_bb));
                if(visit.find(succ_bb) == visit.end()){
                    work_list.push_back(succ_bb);
                }
            }
            visit.insert(bb);
        }
        cur_fun = fun;
        index = 0;
        Tarjan(fun->get_entry_block());
    }
    print_loop();
}

void LoopSearch::print_loop(){
    for(auto x: fun_loop){
        int i = 0;
        std::cout<< "in function: " << x.first->get_name() << std::endl;
        for(auto loop: x.second){
            std::cout << "Loop " << i << ":" << std::endl;
            std::cout << "entry block: " << loop->get_loop_entry()->get_name() << std::endl;
            for(auto bb: loop->get_loop()){
                std::cout<< bb->get_name() << std::endl;
            }
            i++;
        }
    }
}

void LoopSearch::Tarjan(BasicBlock *bb) {
    DFN[bb] = index;
    Low[bb] = index;
    index ++;
    loop_stack.push(bb);
    in_stack.insert(bb);
    visited.insert(bb);
    for(auto edge: edges){
        if(edge.first != bb) continue;
        if(visited.find(edge.second) == visited.end()){
            // not visited
            Tarjan(edge.second);
            Low[bb] = std::min(Low[bb], Low[edge.second]);
        }
        else if(in_stack.find(edge.second) != in_stack.end()){
            // still in stack
            Low[bb] = std::min(Low[bb], DFN[edge.second]);
        }
    }
    if(DFN[bb] == Low[bb]){
        BasicBlock* v = loop_stack.top();
        loop_stack.pop();
        in_stack.erase(v);
        Loop *new_loop = new Loop();
        new_loop->add_loop_block(v);
        std::cout<< v->get_name() <<std::endl;
        while(bb != v){
            v = loop_stack.top();
            loop_stack.pop();
            in_stack.erase(v);
            new_loop->add_loop_block(v);
            std::cout << v->get_name() << std::endl;
        }
        new_loop->set_entry_block(bb);
        fun_loop[cur_fun].insert(new_loop);
    }
}