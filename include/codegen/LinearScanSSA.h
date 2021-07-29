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

struct BlockIDRange {
    int from;
    int to;

    BlockIDRange(int f, int t) : from(f), to(t) {}
};

class Interval {
    std::vector<std::pair<int, int>> _intervals;
    int _begin;
    int _end;

public:
    void addRange(int from, int to);
    void addRange(const BlockIDRange &br);
    void setFrom(int from);

    int getBegin();
    int getEnd();
    bool cover(int pos);

    bool operator< (const Interval &rhs) {
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
};

struct LiveData {
private:
    std::set<Value *> _live;
public:
    void unionLive(const std::unordered_set<Value *> &lv) {
        for (auto v: lv) {
            _live.insert(v);
        }
    }

    void unionLive(const LiveData &lv) {
        for (auto v: lv._live) {
            _live.insert(v);
        }
    }

    std::set<Value *>::iterator begin() {
        return _live.begin();
    }

    std::set<Value *>::iterator end() {
        return _live.end();
    }

    void erase(Value *v) { _live.erase(v); }
    void add(Value *v) { _live.insert(v); }
};

/**
 * Linear Scan 核心算法
 */
class LinearScanSSA {
public:


private:
    // Analysis pass
    CFG _cfg;
    LoopSearch _lp;

    std::map<Instruction *, int> _inst_id;
    std::map<BasicBlock *, BlockIDRange> _block_id;
    std::map<Loop *, BlockIDRange> _loop_id;
    std::map<BasicBlock *, LiveData> _live;
    std::map<Value *, Interval> _interval;
    // We use this class to transfer data between methods
    BBOrderGenerator _BG;
    void assignOpID(Function *f);
    void buildIntervals();
    void linearScan();
};

#endif //BAZINGA_COMPILER_LINEARSCAN_H
