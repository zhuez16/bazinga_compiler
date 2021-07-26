#include <iostream>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "codegen.h"
#include "instgen.h"

#define R 12

bool phyregs[R];

class LiveInterval {

private:
  int begin_;
  int end_;
  int reg_;
  Value * v_;

public:
  explicit LiveInterval(int begin, int end, int reg,Value *v)
      : begin_(begin), end_(end), reg_(reg), v_(v){}

    bool overlapsFrom(LiveInterval *I)
    {
        if(begin_ > I->end || end_ < I->begin)
            return false;
        return true;
    }

    int get_begin()
    {
        return begin_;
    }
    void set_begin(int begin)
    {
        begin_ = begin;
    }
    int get_end()
    {
        return end_;
    }
    void set_end(int end)
    {
        end_ = end;
    }
    void set_reg(int reg)
    {
        reg_ = reg;
    }
    int get_reg()
    {
        return reg_;
    }
    Value * get_value()
    {
        return v_;
    }
    void set_value(Value * v)
    {
        v_ = v;
    }
};

std::vector<LiveInterval*> unhandled;
std::vector<LiveInterval*> active;

bool increase(const LiveInterval *v1, const LiveInterval *v2)
{
    return v1->get_begin() < v2->get_begin();
}
bool decrease(const LiveInterval *v1, const LiveInterval *v2)
{
    return v1->get_end() > v2->get_end();
}

std::map<Value *, int> codegen::regAlloc() {
    this->spill_cost_total = 0;
    this->color_bonus_total = 0;
    this->context_active_vars.clear();
    const double store_cost = 8;
    const double load_cost = 16;
    const double alloca_cost = 2;
    const double mov_cost = 1;
    const double loop_scale = 100;

    std::map<Value *,int> reg_mapping;
    //LoopFind lf(this->module.get());
    //lf.run();
    for (auto &func : this->module->get_functions()) {
        std::map<Value *, std::set<Value *>> IG;
        std::map<Value *, double> spill_cost;
        std::map<Value *, std::map<Value *, double>> phi_bonus;
        std::map<Value *, std::map<int, double>> abi_bonus;
        std::map<Value *, double> weight;
        std::map<Instruction *, int> pos;
        int cur_pos = 1;
        //std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
        std::map<Value *, int> interval_begin,interval_end;
        std::set<Value *> values;
        // not a declaration
        if (func->get_basic_blocks().empty())
            continue;
        // find all vars
        for (auto &args : func->get_args())
            values.insert(args);
        for (auto &bb : func->get_basic_blocks()) {
            for (auto &inst : bb->get_instructions())
                if (inst->get_type()->get_size() > 0)
                    values.insert(inst);
        }
        /*
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
        */
        std::vector<Value *> live_begin;
        std::vector<Value *> live_end;
        std::vector<Value *> live;
        std::vector<BasicBlock *> S;
        std::map<BasicBlock *,int>visit;
        std::map<Value *,LiveInterval *>value_map;
        auto entry_live_in = active_vars::getLiveIn(func->get_entry_block());
        for(auto &op : entry_live_in)
        {
            LiveInterval tmp(0, -1, -1,op);
            unhandled.push_back(&tmp);
            value_map[op]=&tmp;
        }
        S.push_back(func->get_entry_block());
        while(!S.empty())
        {
            auto &bb = S.back();
            S.pop_back();
            live_begin.clear();
            live_end.clear();
            ilve.clear();
            auto live_in= active_vars::getLiveIn(bb);
            auto live_out = active_vars::getLiveOut(bb);
            for(auto &tmp : live_in)
            {
                if(live_out.find(tmp))
                    live.insert(tmp);
                else
                    live_end.insert(tmp);
            }
            for(auto &tmp : live_out)
            {
                if(live_in.find(tmp))
                    continue;
                else
                    live_begin.insert(tmp);
            }
            int in_pos = cur_pos;
            for (auto &inst : bb->get_instructions())
            {
                inst[inst] = cur_pos;
                cur_pos++;
            }
            for (auto &inst : bb->get_instructions())
            {
                int pos = inst[inst];
                auto ops = inst->get_operands();
                for(auto &op : live_begin)
                {
                    if(ops.find(op))
                    {
                        if(value_map.find(op))
                        {
                            LiveInterval * tmp = value_map[op];
                            if(tmp->get_begin() > pos)
                                tmp->set_begin(pos);
                            else if(tmp->get_begin() == -1)
                                tmp->set_begin(pos);
                        }
                        else
                        {
                            LiveInterval tmp(pos, -1, -1,op);
                            unhandled.push_back(&tmp);
                            value_map[op]=&tmp;
                        }
                        live_begin.erase(op);
                    }
                }
                for(auto &op : live_end)
                {
                    LiveInterval * tmp = value_map[op];
                    if(tmp->get_end() == -1)
                        tmp->set_end(pos);
                    else if(tmp->get_end()<pos)
                        tmp->set_end(pos);
                }
                inst[inst] = cur_pos;
                cur_pos++;
            }
            int out_pos = cur_pos-1;
            for(auto &v : live)
            {
                if(value_map.find(v))
                {
                    LiveInterval * tmp = value_map[v];
                    if(tmp->get_begin() > in_pos)
                        tmp->set_begin(in_pos);
                    else if(tmp->get_begin() == -1)
                        tmp->set_begin(in_pos);
                    if(tmp->get_end() == -1)
                        tmp->set_end(out_pos);
                    else if(tmp->get_end()<out_pos)
                        tmp->set_end(out_pos);
                }
                else
                {
                    LiveInterval tmp(in_pos, out_pos, -1,v);
                    unhandled.push_back(&tmp);
                    value_map[v]=&tmp;
                }
            }
            for (auto &succ_bb : bb->get_succ_basic_blocks()) {
                if(visit[succ_bb]==0)
                {
                    S.push_back(succ_bb);
                    visit[succ_bb]=1;
                }
            }
        }
        LinearScanRegisterAllocation();
        for(LiveInterval cur = unhandled.begin(),  e = unhandled.end(); i != e; ++i)
            if(cur.get_reg()!=-1)
                reg_mapping[cur.get_value()]=cur.get_reg();
    }
}

void LinearScanRegisterAllocation()
{
    std::sort(unhandled.begin(), unhandled.end(), increase);
    reg_mapping.clear();
    active.clear();
    for(int i=0;i<R;i++)
        regs[i]=true;
    for(LiveInterval cur = unhandled.begin(),  e = unhandled.end(); i != e; ++i)
    {
        ExpireOldIntervals(&cur);
        if(active.size()==R)
            SpillAtInterval(&cur);
        else
        {
           active.push_back(&cur);
            for(int i=0;i<R;i++)
                if(regs[i])
                {
                    cur.set_reg(i);
                    break;
                }
        }
    }
}

void ExpireOldIntervals(LiveInterval *cur)
{
    std::sort(active.begin(), active.end(), decrease);
    for(LiveInterval i = active.begin(),  e = active.end(); i != e; ++i)
    {
        if(i.get_end()>=cur->get_begin())
            return;
        regs[i.get_reg()]=true;
        active.erase(i);
    }
}
void SpillAtInterval(LiveInterval * cur)
{
    std::sort(active.begin(), active.end(), decrease);
    LiveInterval* spill = unhandled.back();
    if(spill->get_end()>cur->get_end())
    {
        cur->set_reg(spill->get_reg());
        spill->set_reg(-1);
        get_stack(spill);
        active.push_back(spill);
        active.push_back(cur);
    }
    else
        get_stack(cur);
}