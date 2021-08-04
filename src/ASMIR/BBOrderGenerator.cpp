//
// Created by 93685 on 2021/8/4.
//

#include "ASMIR/BBOrderGenerator.h"
#include "ASMIR/ASValue.hpp"

void BBOrderGenerator::clearQueue() {
    std::queue<BasicBlock *> empty_queue;
    std::swap(_visit_queue, empty_queue);
    _visited.clear();
    _queue.clear();
}

bool BBOrderGenerator::visited(BasicBlock *bb) {
    return _visited.find(bb) != _visited.end();
}

void BBOrderGenerator::runOnFunction(Function *f) {
    if (f->is_declaration()) return;
    clearQueue();
    _cfg->runOnFunction(f);

    // BFS遍历BB，若遇到循环则优先处理循环中的块
    _visit_queue.push(f->get_entry_block());
    while (!_visit_queue.empty()) {
        auto bb = _visit_queue.front();
        _visit_queue.pop();
        if (visited(bb)) continue;
        // 若在循环中，则交给循环处理模块进行处理
        Loop *loop = _lp->get_smallest_loop(bb);
        if (loop != nullptr) {
            runOnLoop(loop);
        } else {
            _visited.insert(bb);
            _queue.push_back(bb);
        }
        for (auto succ: _cfg->getSuccBB(bb)) {
            if (succ != bb) {
                _visit_queue.push(succ);
            }
        }
    }
}

void BBOrderGenerator::runOnLoop(Loop *loop) {
    std::queue<BasicBlock *> visit_queue;
    visit_queue.push(loop->get_loop_entry());
    while (!visit_queue.empty()) {
        auto bb = visit_queue.front();
        visit_queue.pop();
        if (visited(bb)) continue;
        if (_lp->get_smallest_loop(bb) != loop) {
            runOnLoop(_lp->get_smallest_loop(bb));
        } else {
            _visited.insert(bb);
            visit_queue.push(bb);
            _queue.push_back(bb);
        }
        for (auto succ: _cfg->getSuccBB(bb)) {
            if (loop->contain_bb(succ) && bb != succ) {
                visit_queue.push(succ);
            }
        }
    }
}

std::vector<ASBlock *> BBOrderGenerator::getInverseASMBBOrder(std::map<Value *, ASValue *> &map) {
    {
        std::vector<ASBlock *> ret;
        for (auto bb: getInverseBBOrder()) {
            auto asm_bb = dynamic_cast<ASBlock *>(map[bb]);
            assert(asm_bb && "Can't get ASM Block by BasicBlock");
            ret.push_back(asm_bb);
        }
        return ret;
    }
}

std::vector<ASBlock *> BBOrderGenerator::getASMBBOrder(std::map<Value *, ASValue *> &map) {
    {
        std::vector<ASBlock *> ret;
        for (auto bb: getBBOrder()) {
            auto asm_bb = dynamic_cast<ASBlock *>(map[bb]);
            assert(asm_bb && "Can't get ASM Block by BasicBlock");
            ret.push_back(asm_bb);
        }
        return ret;
    }
}