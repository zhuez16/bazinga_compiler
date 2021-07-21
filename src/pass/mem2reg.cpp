#include <IR/BasicBlock.h>
#include "Pass/mem2reg.h"
#include "Pass/dominator.h"
void Mem2RegPass::run() {
    printf("running mem2reg\n");
    for(auto f: m_->get_functions()){
        // get dominance frontier message
        dom = new dominator(m_);
        printf("before run dom\n");
        dom->run();
        printf("after run dom\n");
        std::set<Value *> promote_alloca;
        std::map<Value *, std::set<BasicBlock *>> alloca_to_live_in_block;
        // record block use but not define
        for(auto bb: f->get_basic_blocks()){
            std::set<Value *> record;
            for(auto instr: bb->get_instructions()){
                // store: store i32 val, i32 ptr
                if(instr->is_store()){
                    auto val = instr->get_operand(0);
                    auto ptr = instr->get_operand(1);
                    if( !(dynamic_cast<GlobalVariable *>(ptr)) &&
                        !(dynamic_cast<GetElementPtrInst *>(ptr))
                        ){
                        if(record.find(val) == record.end()){
                            promote_alloca.insert(ptr);
                        }
                        record.insert(ptr);
                        alloca_to_live_in_block[ptr].insert(bb);
                    }
                }
            }
        }
        std::set<std::pair<BasicBlock *, Value *>> block_own_phi; // bb has phi for var
        for(auto alloca: promote_alloca ){
            std::vector<BasicBlock *> work_list;
            for(auto bb: alloca_to_live_in_block[alloca]){
                work_list.push_back(bb);
            }
            while(!work_list.empty()){
                auto bb = work_list.back();
                work_list.pop_back();
                for(auto dominance_frontier_bb: dom->get_dominance_frontier(bb)){
                    auto new_pair = std::make_pair(dominance_frontier_bb, alloca);
                    if(block_own_phi.find(new_pair) != block_own_phi.end()){
                        auto phi = PhiInst::create_phi(alloca->get_type()->get_pointer_element_type(), dominance_frontier_bb);
                        phi->set_lval(alloca);
                        dominance_frontier_bb->add_instr_begin(phi);
                        work_list.push_back(dominance_frontier_bb);
                        block_own_phi.insert({dominance_frontier_bb, alloca});
                    }
                }
            }
        }

    }
}
