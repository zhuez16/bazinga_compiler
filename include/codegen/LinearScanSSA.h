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

#define NUM_REG 10

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
    ASValue *_v = nullptr;

public:
    std::string toString(RegMapper *mapper) const {
        std::string ret = mapper->getName(nullptr, _v);
        ret += "  Reg: " + std::to_string(_reg);
        ret += "  Spi: " + std::to_string(_spill);
        ret += "  Range: ";
        for (auto iv: _intervals) {
            ret += "[" + std::to_string(iv.first) + " - " + std::to_string(iv.second) + "] ";
        }
        ret += "\n";
        return ret;
    }

    Interval(ASValue *v, int pos) : _reg(-1), _spill(-1), _begin(pos), _end(pos), _v(v) {
        _intervals.emplace_back(pos, pos);
    }

    Interval() : _reg(-1), _begin(INT_MAX), _end(-1), _spill(-1) {
        _intervals.emplace_back(INT_MAX, -1);
    }
    Interval(int pos, int reg) : _reg(reg), _begin(pos), _end(pos), _spill(-1) {
        _intervals.emplace_back(pos, pos);
    }
    Interval(int from, int to, int reg) : _reg(reg), _begin(from), _end(to), _spill(-1) {
        _intervals.emplace_back(from, to);
    }

    void setValue(ASValue *v) { _v = v; }
    void addRange(int from, int to);
    void addRange(const BlockIDRange &br) { addRange(br.from,br.to); }
    void setFrom(int from){
        this->_begin=from;
        this->_intervals.front().first=from;
    }
    void setSpill(int spillSlot){
        for (auto it:this->_intervals){
            if (it.first < spillSlot && it.second > spillSlot){
                addRange(it.first,spillSlot-1);
                addRange(spillSlot,it.second);
                std::remove(this->_intervals.begin(),this->_intervals.end(),it);
                break;
            }
        }
        std::sort(this->_intervals.begin(),this->_intervals.end());
    }
    void setRegister(int regId){
        this->_reg=regId;
    }

    ASValue *getValue() { return _v; }
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
        setSpill(position);
        Interval P1;
        for (auto it: this->_intervals){
            if (it.first>=position){
                P1._intervals.push_back(it);
                P1._end=it.second;
            }
        }
        P1._begin=position;
        while (this->_intervals.back().first >= position) this->_intervals.pop_back();
        this->_end=position-1;
        P1.setValue(getValue());
        return P1;
    }

    int getBegin() const {return this->_begin;};
    int getEnd() const {return this->_end;};
    int getRegister() const {return this->_reg;};
    int getNextUse(int after) const {
        for (auto it: this->_intervals){
            if (it.first >= after)
                return it.first;
        }
        return -1;
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

    bool operator< (const Interval &rhs) const {
        return _begin < rhs._begin;
    }
};

/**
 * 生成BB顺序，保证:
 *  1. 一个循环的所有块是连续的
 *  2. 所有块的支配块在这个块之前出现
 */
class BBOrderGenerator {
private:
    std::set<BasicBlock *> _visited;
    std::vector<BasicBlock *> _queue;
    std::queue<BasicBlock *> _visit_queue;
    void runOnLoop(Loop *loop);
    bool visited(BasicBlock *bb);
    void clearQueue();
    CFG *_cfg;
    LoopSearch *_lp;
public:
    BBOrderGenerator(Module *m) : _cfg(new CFG(m)), _lp(new LoopSearch(m)) {
        _lp->run();
    }
    void runOnFunction(Function *f);

    std::vector<BasicBlock *> getBBOrder() {
        return _queue;
    }

    std::vector<BasicBlock *> getInverseBBOrder() {
        auto ret = getBBOrder();
        std::reverse(ret.begin(), ret.end());
        return ret;
    }

    std::vector<ASBlock *> getASMBBOrder(std::map<Value *, ASValue *> &map) {
        std::vector<ASBlock *> ret;
        for (auto bb: getBBOrder()) {
            auto asm_bb = dynamic_cast<ASBlock *>(map[bb]);
            assert(asm_bb && "Can't get ASM Block by BasicBlock");
            ret.push_back(asm_bb);
        }
        return ret;
    }

    std::vector<ASBlock *> getInverseASMBBOrder(std::map<Value *, ASValue *> &map) {
        std::vector<ASBlock *> ret;
        for (auto bb: getInverseBBOrder()) {
            auto asm_bb = dynamic_cast<ASBlock *>(map[bb]);
            assert(asm_bb && "Can't get ASM Block by BasicBlock");
            ret.push_back(asm_bb);
        }
        return ret;
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
    void run(ASMBuilder *builder, Module *m) {
        map = builder->getValueMap();
        delete _cfg;
        delete _lp;
        delete BG;
        _cfg = new CFG(m);
        _lp = new LoopSearch(m);
        BG = new BBOrderGenerator(m);
        _lp->run();
        for (auto f: m->get_functions()) {
            if (!f->is_declaration()) {
                _cfg->runOnFunction(f);
                BG->runOnFunction(f);
                auto asmF = builder->getMapping<ASFunction>(f);
                currentFunc = asmF;
                assignOpID(f, asmF);
                buildIntervals();
                linearScan();

                for (auto it: handled) {

                }
            }
        }
    }

    std::vector<Interval> getInterval() { return handled; };


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
    BBOrderGenerator *BG;
    void assignOpID(Function *of, ASFunction *f);
    void buildIntervals();
    void linearScan();
    bool tryAllocateFreeRegister(Interval &current, int position);
    void allocateBlockedRegister(Interval &current, int position);
    int requireNewSpillSlot(ASValue *v) { return currentFunc->allocStack(v, 1); };
};

#endif //BAZINGA_COMPILER_LINEARSCAN_H
