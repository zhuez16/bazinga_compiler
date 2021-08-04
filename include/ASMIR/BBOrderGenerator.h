//
// Created by 93685 on 2021/8/4.
//

#ifndef BAZINGA_COMPILER_BBORDERGENERATOR_H
#define BAZINGA_COMPILER_BBORDERGENERATOR_H

#include <set>
#include <queue>
#include <algorithm>

#include "pass/loop_search.h"
#include "pass/CFG.h"

class BasicBlock;
class ASValue;
class ASBlock;

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
    explicit BBOrderGenerator(Module *m) : _cfg(new CFG(m)), _lp(new LoopSearch(m)) {
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

    std::vector<ASBlock *> getASMBBOrder(std::map<Value *, ASValue *> &map);

    std::vector<ASBlock *> getInverseASMBBOrder(std::map<Value *, ASValue *> &map);
};

#endif //BAZINGA_COMPILER_BBORDERGENERATOR_H
