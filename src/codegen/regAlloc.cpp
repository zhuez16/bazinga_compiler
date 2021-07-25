#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "CodeGen.hh"
#include "InstructionsGen.hh"
#include "LoopFind.h"

std::map<Value *, int> CodeGen::regAlloc() {
    this->spill_cost_total = 0;
    this->color_bonus_total = 0;
    this->context_active_vars.clear();
    const double store_cost = 8;
    const double load_cost = 16;
    const double alloca_cost = 2;
    const double mov_cost = 1;
    const double loop_scale = 100;
    std::map<Value *, int> mapping;
    LoopFind lf(this->module.get());
    lf.run();
    for (auto &func : this->module->getFunctions()) {
        std::map<Value *, std::set<Value *>> IG;
        std::map<Value *, double> spill_cost;
        std::map<Value *, std::map<Value *, double>> phi_bonus;
        std::map<Value *, std::map<int, double>> abi_bonus;
        std::map<Value *, double> weight;
        std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
        std::set<Value *> values;
        bool mt_inside = CodeGen::is_mt_inside(func);
        // not a declaration
        if (func->getBasicBlocks().empty()) {
        continue;
        }
        // find all vars
        for (auto &args : func->getArgs()) {
        values.insert(args);
        }
        for (auto &bb : func->getBasicBlocks()) {
        for (auto &inst : bb->getInstructions()) {
            if (inst->getType()->getSize() > 0) {
            values.insert(inst);
            }
        }
        }
        // calc live in
        {
        for (auto &v : values) {
            std::queue<BasicBlock *> Q;
            if (!dynamic_cast<Instruction *>(v)) {
            live_in[func->getEntryBlock()].insert(v);
            Q.push(func->getEntryBlock());
            live_in[Q.front()].insert(v);
            } else {
            auto bb = dynamic_cast<Instruction *>(v)->getParent();
            for (auto &succ_bb : bb->getSuccBasicBlocks()) {
                Q.push(succ_bb);
                live_in[succ_bb].insert(v);
            }
            }
            auto banned = nullptr; // no banned
            while (!Q.empty()) {
            auto x = Q.front();
            Q.pop();
            for (auto &succ_bb : x->getSuccBasicBlocks()) {
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
        for (auto &bb : func->getBasicBlocks()) {
            for (auto &inst : bb->getInstructions()) {
            for (auto &op : inst->getOperands()) {
                if (!values.count(op)) {
                continue;
                }
                std::queue<BasicBlock *> Q;
                if (inst->isPHI()) {
                int cnt = 0;
                Value *pre_op = nullptr;
                for (auto &op_phi : inst->getOperands()) {
                    if (pre_op == op) {
                    assert(dynamic_cast<BasicBlock *>(op_phi));
                    auto x = static_cast<BasicBlock *>(op_phi);
                    Q.push(x);
                    live_out[x].insert(op);
                    }
                    pre_op = op_phi;
                }
                } else {
                bool flag = false;
                for (auto inst_prev : bb->getInstructions()) {
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
                for (auto &prev_bb : bb->getPreBasicBlocks()) {
                    Q.push(prev_bb);
                    live_out[prev_bb].insert(op);
                }
                }
                while (!Q.empty()) {
                auto x = Q.front();
                Q.pop();
                bool flag = false;
                for (auto &inst_prev : x->getInstructions()) {
                    if (inst_prev == op) {
                    flag = true;
                    break;
                    }
                }
                if (flag) {
                    continue;
                }
                for (auto &prev_bb : x->getPreBasicBlocks()) {
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