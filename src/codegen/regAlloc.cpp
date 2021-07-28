#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "codegen/codegen.h"
#include "codegen/instgen.h"
#include "pass/loop_search.h"

std::map<Value *, int> codegen::regAlloc() {
    this->active_vars.clear();
    const double store_cost = 8;
    const double load_cost = 16;
    const double alloca_cost = 2;
    const double mov_cost = 1;
    const double loop_scale = 100;
    std::map<Value *, int> mapping;
    LoopSearch lf(this->module);
    lf.run();
    for (auto &func : this->module->get_functions()) {
        std::map<Value *, std::set<Value *>> IG;
        std::map<Value *, double> spill_cost;
        std::map<Value *, std::map<Value *, double>> phi_bonus;
        std::map<Value *, std::map<int, double>> abi_bonus;
        std::map<Value *, double> weight;
        std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
        std::set<Value *> values;
        // not a declaration
        if (func->get_basic_blocks().empty()) {
        continue;
        }
        // find all vars
        for (auto &args : func->get_args()) {
        values.insert(args);
        }
        for (auto &bb : func->get_basic_blocks()) {
        for (auto &inst : bb->get_instructions()) {
            if (inst->get_type()->get_size() > 0) {
            values.insert(inst);
            }
        }
        }
        // calc live in
        {
        for (auto &v : values) {
            std::queue<BasicBlock *> Q;
            if (!dynamic_cast<Instruction *>(v)) {
            live_in[func->get_entry_block()].insert(v);
            Q.push(func->get_entry_block());
            live_in[Q.front()].insert(v);
            } else {
            auto bb = dynamic_cast<Instruction *>(v)->get_parent();
            for (auto &succ_bb : bb->get_succ_basic_blocks()) {
                Q.push(succ_bb);
                live_in[succ_bb].insert(v);
            }
            }
            auto banned = nullptr; // no banned
            while (!Q.empty()) {
            auto x = Q.front();
            Q.pop();
            for (auto &succ_bb : x->get_succ_basic_blocks()) {
                if (succ_bb != banned && !live_in[succ_bb].count(v)) {
                live_in[succ_bb].insert(v);
                Q.push(succ_bb);
                }
            }
            }
        }
        }
        // calc live out
        {
            for (auto &bb : func->get_basic_blocks()) {
                for (auto &inst : bb->get_instructions()) {
                    for (auto &op : inst->get_operands()) {
                        if (!values.count(op)) {
                        continue;
                        }
                        std::queue<BasicBlock *> Q;
                        if (inst->is_phi()) {
                            int cnt = 0;
                            Value *pre_op = nullptr;
                            for (auto &op_phi : inst->get_operands()) {
                                if (pre_op == op) {
                                    assert(dynamic_cast<BasicBlock *>(op_phi));
                                    auto x = static_cast<BasicBlock *>(op_phi);
                                    Q.push(x);
                                    live_out[x].insert(op);
                                }
                                pre_op = op_phi;
                            }
                        }
                        else {
                            bool flag = false;
                            for (auto inst_prev : bb->get_instructions()) {
                                if (inst_prev == inst) {
                                break;
                                }
                                if (inst_prev == op) {
                                flag = true;
                                break;
                                }
                            }
                            if (flag) {
                                continue;
                            }
                            for (auto &prev_bb : bb->get_pre_basic_blocks()) {
                                Q.push(prev_bb);
                                live_out[prev_bb].insert(op);
                            }
                        }
                        while (!Q.empty()) {
                            auto x = Q.front();
                            Q.pop();
                            bool flag = false;
                            for (auto &inst_prev : x->get_instructions()) {
                                if (inst_prev == op) {
                                flag = true;
                                break;
                                }
                            }
                            if (flag) {
                                continue;
                            }
                            for (auto &prev_bb : x->get_pre_basic_blocks()) {
                                if (!live_out[prev_bb].count(op)) {
                                    Q.push(prev_bb);
                                    live_out[prev_bb].insert(op);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}