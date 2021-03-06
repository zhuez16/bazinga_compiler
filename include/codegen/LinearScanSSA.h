//
// Created by 顾超 on 2021/7/29.
// Linear Scan Algorithm on SSA
// Reference: Linear scan register allocation on SSA form  https://dl.acm.org/doi/abs/10.1145/1772954.1772979
//

#ifndef BAZINGA_COMPILER_LINEARSCAN_H
#define BAZINGA_COMPILER_LINEARSCAN_H

#include <vector>
#include <set>
#include <algorithm>
#include "IR/BasicBlock.h"
#include "IR/Instruction.h"
#include "pass/CFG.h"
#include "pass/dominator.h"
#include "pass/loop_search.h"
#include "pass/active_vars.h"
#include "ASMIR/ASMBuilder.h"
#include "ASMIR/RegAllocMapper.h"
#include "ASMIR/BBOrderGenerator.h"
#include <climits>
#define NUM_REG 10

class RegMapper;
class ASValue;
class ASFunction;
class ASBlock;
class ASInstruction;
class ASMBuilder;

struct BlockIDRange {
    int from;
    int to;
    BlockIDRange() = default;
    BlockIDRange(int f, int t) : from(f), to(t) {}
};

class Interval {
    std::vector<std::pair<int, int>> _intervals;
    int _begin;
    int _end;
    int _reg;
    int _spill;
    bool _fixed = false;
    ASValue *_v = nullptr;

public:
    std::string toString(RegMapper *mapper) const;
    Interval(ASValue *v, int pos) : _reg(-1), _spill(-1), _begin(pos), _end(pos), _v(v) {
        // _intervals.emplace_back(pos, pos);
    }

    Interval() : _reg(-1), _begin(INT_MAX), _end(-1), _spill(-1) {
        _intervals.emplace_back(INT_MAX, -1);
    }
    Interval(int pos, int reg, ASValue *v= nullptr, bool fixed= false) : _reg(reg), _begin(pos), _end(pos), _spill(-1), _v(v), _fixed(fixed) {
        assert(pos > 0 && "Invalid position");
        _intervals.emplace_back(pos, pos);
    }
    Interval(int from, int to, int reg, ASValue *v= nullptr) : _reg(reg), _begin(from), _end(to), _spill(-1), _v(v) {
        assert(from > 0 && to > from && "Invalid range");
        _intervals.emplace_back(from, to);
    }

    void setValue(ASValue *v) { _v = v; }
    void addRange(int from, int to);
    void addRange(const BlockIDRange &br) { addRange(br.from,br.to); }
    void setFrom(int from){
        _begin=from;
        if (this->_intervals.empty()) {
            _intervals.emplace_back(_begin, _end);
        } else {
            _intervals.front().first = from;
        }
    }
    void setSpill(int spillSlot){
        _spill = spillSlot;
    }
    void setRegister(int regId){
        this->_reg=regId;
    }

    ASValue *getValue() const { return _v; }
    std::vector<std::pair<int,int>> getIntervals(){ return _intervals; }

    /**
     * 将当前块划分为两部分
     * P1: start ~ position - 1
     * P2: position ~ end
     * 自身上下界变为P1
     * 返回的Interval为P2
     * @param position
     * @return
     */
    Interval split(int position){
        assert(!isFixed() && "Fixed interval is not splittable");
        // if (position < 0) assert(0);
        // setSpill(position);
        Interval P1;
        for (auto it = _intervals.begin(); it != _intervals.end(); ++it){
            if (it->first <= position && it->second >= position && it->first!=it->second){
//                addRange(it->first,position-1);
//                addRange(position,it->second);
                while (_intervals.back().first != it->first){
                    P1._intervals.insert(P1._intervals.begin(),_intervals.back());
                    _intervals.pop_back();
                }
                auto pair1=std::make_pair(it->first,position-1);
                _intervals.pop_back();
                _intervals.push_back(pair1);
                P1.addRange(position,this->_end);
                this->_end=position-1;
                break;
            }
        }
        P1.setValue(getValue());
        // P1._begin=position;
//        while (this->_intervals.back().first >= position) this->_intervals.pop_back();
//        this->_end=position-1;
//        P1.setValue(getValue());
//        P1.setRegister(-1);
//        P1.setSpill(-1);
        return P1;
    }

    int getBegin() const {return this->_begin;};
    int getEnd() const {return this->_end;};
    int getRegister() const {return this->_reg;};
    int getSpill() const {return this->_spill;}
    int getNextUse(int after, const std::map<ASInstruction *, int> &instIdMap) const {
        int nextUse = INT_MAX;
        for (const auto &u: getValue()->getUseList()) {
            auto val = u._user;
            if (dynamic_cast<ASInstruction *>(val)) {
                int pos = instIdMap.at(dynamic_cast<ASInstruction *>(val));
                if (pos > after) {
                    nextUse = std::min(nextUse, pos);
                }
            }
        }
        /*
        for (auto it: this->_intervals){
            if (it.first > after)
                return it.first;
        }
         */
        return nextUse;
    };
    bool cover(int pos) const {
        for (auto it: this->_intervals){
            if (it.first <= pos && pos <= it.second)
                return true;
        }
        return false;
    };
    bool intersect(const Interval &current) const {
        for (auto it_cur:current._intervals){
            for (auto it_this:this->_intervals){
                if (it_cur.first < it_this.second && it_cur.second > it_this.first){
                    return true;
                }
            }
        }
        return false;
    };
    int intersect(int pos, const Interval &current){
        for (auto it_cur:current._intervals){
            for (auto it_this:this->_intervals){
                if (it_cur.first < it_this.second && it_cur.second > it_this.first){
                    if (it_cur.first > pos)
                        return it_cur.first;
                }
            }
        }
        return 0;
    }

    bool isFixed() const { return _fixed; }

    bool operator< (const Interval &rhs) const {
        return _begin < rhs._begin;
    }
};

struct LiveData {
private:
    std::set<ASValue *> _live;
public:
    void unionLive(const std::unordered_set<ASValue *> &lv) {
        for (auto v: lv) {
            _live.insert(v);
        }
    }

    void unionLive(const LiveData &lv) {
        for (auto v: lv._live) {
            _live.insert(v);
        }
    }

    std::set<ASValue *>::iterator begin() {
        return _live.begin();
    }

    std::set<ASValue *>::iterator end() {
        return _live.end();
    }

    void erase(ASValue *v) { _live.erase(v); }
    void add(ASValue *v) { _live.insert(v); }
};

/**
 * Linear Scan 核心算法
 */
class LinearScanSSA {
public:
    void run(ASMBuilder *builder, Module *m);

    std::vector<Interval> getIntervals() { return handled; };

    std::map<ASInstruction *, int>  getInstId () {return _inst_id;}

    LinearScanSSA() = default;

private:
    // Analysis pass
    CFG *_cfg = nullptr;
    LoopSearch *_lp = nullptr;

    std::map<ASInstruction *, int> _inst_id;
    std::map<ASBlock *, BlockIDRange> _block_id;
    std::map<Loop *, BlockIDRange> _loop_id;
    std::map<ASBlock *, LiveData> _live;
    std::map<ASValue *, Interval> _interval;
    // Data structure for linear scan algo
    std::vector<Interval> unhandled;
    std::vector<Interval> active;
    std::vector<Interval> inactive;
    std::vector<Interval> handled;
    // A fix position for function call
    std::vector<Interval> fixed;
    // Mapping provided by ASM Builder
    std::map<Value *, ASValue*> map;
    // We use this class to transfer data between methods
    ASFunction *currentFunc = nullptr;
    BBOrderGenerator *BG = nullptr;

    std::map<ASInstruction *, int> get_inst_id(){return _inst_id;}
    void assignOpID(Function *of, ASFunction *f);
    void buildIntervals();
    void linearScan();
    bool tryAllocateFreeRegister(Interval &current, int position);
    void allocateBlockedRegister(Interval &current, int position);
    int requireNewSpillSlot(ASValue *v);
};

#endif //BAZINGA_COMPILER_LINEARSCAN_H
