#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include "codegen/codegen.h"
#include "codegen/instgen.h"
#include "pass/loop_search.h"
#include "pass/active_vars.h"

#define R 12

bool regs[R]={true,true,true,true,true,true,true,true,true,true,true,true};
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
        if(begin_ > I->end_ || end_ < I->begin_)
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

bool increase(LiveInterval *v1, LiveInterval *v2)
{
    return v1->get_begin() < v2->get_begin();
}
bool decrease(LiveInterval *v1, LiveInterval *v2)
{
    return v1->get_end() > v2->get_end();
}


void SpillAtInterval(LiveInterval * cur)
{
    std::sort(active.begin(), active.end(), decrease);
    LiveInterval* spill = unhandled.back();
    if(spill->get_end()>cur->get_end())
    {
        cur->set_reg(spill->get_reg());
        spill->set_reg(-1);
//        get_stack(spill);
        active.push_back(spill);
        active.push_back(cur);
    }
//    else
//        get_stack(cur);
}
void ExpireOldIntervals(LiveInterval *cur)
{
    std::sort(active.begin(), active.end(), decrease);
    for(auto i:active)
    {
        if(i->get_end()>=cur->get_begin())
            return;
        regs[i->get_reg()]=true;
        std::remove(active.begin(),active.end(),i);
    }
}
void LinearScanRegisterAllocation()
{
    std::sort(unhandled.begin(), unhandled.end(), increase);
    active.clear();
    for(int i=0;i<R;i++)
        regs[i]=true;
    for(auto cur:unhandled)
    {
        ExpireOldIntervals(cur);
        if(active.size()==R)
            SpillAtInterval(cur);
        else
        {
            active.push_back(cur);
            for(int i=0;i<R;i++)
                if(regs[i])
                {
                    cur->set_reg(i);
                    regs[i] = false;
                    break;
                }
        }
    }
}


std::map<Value *, int> codegen::regAlloc() {
    std::map<Value *,int> reg_mapping;
    ActiveVars bbactive=ActiveVars(module);
    bbactive.run();

    //LoopFind lf(this->module.get());
    //lf.run();
    for (auto &func : this->module->get_functions()) {
        std::map<Instruction *, int> pos;
        int cur_pos = 1;
        //std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
        std::map<Value *, int> interval_begin,interval_end;
//        std::set<Value *> values;
        // not a declaration
        if (func->get_basic_blocks().empty())
            continue;
//        // find all vars
//        for (auto &args : func->get_args())
//            values.insert(args);
//        for (auto &bb : func->get_basic_blocks()) {
//            for (auto &inst : bb->get_instructions())
//                if (inst->get_type()->get_size() > 0)
//                    values.insert(inst);
//        }
        unhandled.clear();
        std::vector<Value *> live_begin;
        std::vector<Value *> live_end;
        std::vector<Value *> live;
        std::vector<BasicBlock *> S;
        std::map<BasicBlock *,int>visit;
        std::map<Value *,LiveInterval *>value_map;
        auto entry_live_in = bbactive.getLiveIn(func->get_entry_block());
        for(auto &op : entry_live_in)
        {
            LiveInterval *tmp=new LiveInterval(0, -1, -1,op);
            unhandled.push_back(tmp);
            value_map[op]=tmp;
        }
        S.push_back(func->get_entry_block());
        while(!S.empty())
        {
            auto &bb = S.back();
            S.pop_back();
            live_begin.clear();
            live_end.clear();
            live.clear();
            auto live_in= bbactive.getLiveIn(bb);
            auto live_out = bbactive.getLiveOut(bb);
            for(auto &op : live_in)
            {
                if(value_map.find(op)!=value_map.end())
                {
                    LiveInterval * tmp = value_map[op];
                    if(tmp->get_begin() > cur_pos)
                        tmp->set_begin(cur_pos);
                    else if(tmp->get_begin() == -1)
                        tmp->set_begin(cur_pos);
                }
                else
                {
                    LiveInterval *tmp=new LiveInterval(cur_pos, -1, -1,op);
                    unhandled.push_back(tmp);
                    value_map[op]=tmp;
                }
                if(live_out.find(op) != live_out.end())
                    live.push_back(op);
                else
                    live_end.push_back(op);
            }
            for(auto &tmp : live_out)
            {
                if(live_in.find(tmp) != live_out.end() )
                    continue;
                else
                    live_begin.push_back(tmp);
            }
            int in_pos = cur_pos;
            for (auto &inst : bb->get_instructions())
            {
                pos[inst] = cur_pos;
                cur_pos++;
            }
            for (auto &inst : bb->get_instructions())
            {
                int pos_ = pos[inst];
                auto ops = inst->get_operands();
                for(auto &op : live_begin)
                {
                    if(std::find(ops.begin(),ops.end(),op)!=ops.end())
                    {
                        if(value_map.find(op)!=value_map.end())
                        {
                            LiveInterval * tmp = value_map[op];
                            if(tmp->get_begin() > pos_)
                                tmp->set_begin(pos_);
                            else if(tmp->get_begin() == -1)
                                tmp->set_begin(pos_);
                        }
                        else
                        {
                            LiveInterval *tmp=new LiveInterval(pos_, -1, -1,op);
                            unhandled.push_back(tmp);
                            value_map[op]=tmp;
                        }
                        std::remove(live_begin.begin(),live_begin.end(),op);
                    }
                }
                for(auto &op : live_end)
                {
                    LiveInterval * tmp = value_map[op];
                    if(tmp->get_end() == -1)
                        tmp->set_end(pos_);
                    else if(tmp->get_end()<pos_)
                        tmp->set_end(pos_);
                }
                pos[inst] = cur_pos;
                cur_pos++;
            }
            int out_pos = cur_pos-1;
            for(auto &v : live)
            {
                if(value_map.find(v)!=value_map.end())
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
                    LiveInterval *tmp=new LiveInterval(in_pos, out_pos, -1,v);
                    unhandled.push_back(tmp);
                    value_map[v]=tmp;
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
        for(auto cur:unhandled)
            if(cur->get_reg()!=-1)
                reg_mapping[cur->get_value()]=cur->get_reg();
    }
    return reg_mapping;
}
