#include <IR/BasicBlock.h>
#include "pass/mem2reg.h"
#include "pass/dominator.h"

std::map<Value *, std::vector<Value *>> var_val_stack;

void Mem2Reg::run() {
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
        re_name(f->get_entry_block());
    }
}
void Mem2Reg::re_name(BasicBlock *bb) {
    std::vector<Instruction *> wait_delete;
    for (auto instr : bb->get_instructions() ){
        if (instr->is_phi()){
            // step 3: push phi instr as lval's lastest value define
            auto l_val = dynamic_cast<PhiInst *>(instr)->get_lval();
            var_val_stack[l_val].push_back(instr);
        }
    }

    for (auto instr : bb->get_instructions() ){
        if ( instr->is_load() ){
            // step 4: replace load with the top of stack[l_val]
            auto l_val = dynamic_cast<LoadInst *>(instr)->get_lval();

            if (!dynamic_cast<GlobalVariable *>(l_val) &&
                !dynamic_cast<GetElementPtrInst *>(l_val)){
                if ( var_val_stack.find(l_val)!=var_val_stack.end()){
                    instr->replace_all_use_with(var_val_stack[l_val].back());
                    wait_delete.push_back(instr);
                }
            }
        }
        if (instr->is_store()){
            // step 5: push r_val of store instr as lval's lastest definition
            auto l_val = dynamic_cast<StoreInst *>(instr)->get_lval();
            auto r_val = dynamic_cast<StoreInst *>(instr)->get_rval();

            if (!dynamic_cast<GlobalVariable *>(l_val) &&
                !dynamic_cast<GetElementPtrInst *>(l_val)){
                var_val_stack[l_val].push_back(r_val);
                wait_delete.push_back(instr);
            }
        }
    }

    for ( auto succ_bb : bb->get_succ_basic_blocks() ){
        for ( auto instr : succ_bb->get_instructions() ){
            if ( instr->is_phi()){
                auto l_val = dynamic_cast<PhiInst *>(instr)->get_lval();
                if (var_val_stack.find(l_val)!= var_val_stack.end()){
//                    assert(var_val_stack[l_val].size()!=0);
                    // step 6: fill phi pair parameters
                    dynamic_cast<PhiInst *>(instr)->add_phi_pair_operand( var_val_stack[l_val].back(), bb);
                }
                // else phi parameter is [ undef, bb ]
            }
        }
    }

    for ( auto dom_succ_bb : dom->get_dom_tree_succ_blocks(bb) ){
        re_name(dom_succ_bb);
    }

    for (auto instr : bb->get_instructions()){
        // step 7: pop lval's lastest definition
        if(instr->is_store()){
            auto l_val = dynamic_cast<StoreInst *>(instr)->get_lval();
            if (!dynamic_cast<GlobalVariable *>(l_val) &&
                !dynamic_cast<GetElementPtrInst *>(l_val)){
                var_val_stack[l_val].pop_back();
            }
        }
        else if (instr->is_phi()){
            auto l_val = dynamic_cast<PhiInst *>(instr)->get_lval();
            if ( var_val_stack.find(l_val)!=var_val_stack.end()){
                var_val_stack[l_val].pop_back();
            }
        }
    }
    for ( auto instr : wait_delete){
        bb->delete_instr(instr);
    }
}