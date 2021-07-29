//
// Created by 顾超 on 2021/7/29.
//

#include "codegen/LinearScanSSA.h"






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
            _visit_queue.push(succ);
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
            runOnLoop(loop);
        } else {
            _visited.insert(bb);
            visit_queue.push(bb);
        }
        for (auto succ: _cfg->getSuccBB(bb)) {
            if (loop->contain_bb(succ)) {
                visit_queue.push(succ);
            }
        }
    }
}

void LinearScanSSA::assignOpID(Function *f) {
    int id = 1;
    for (auto bb: _BG.getBBOrder()) {
        int begin = id;
        for (auto inst: bb->get_instructions()) {
            _inst_id[inst] = id++;
        }
        int end = id - 1;
        _block_id[bb] = BlockIDRange(begin, end);
    }
    for (auto loop: _lp.get_loop(f)) {
        int start = INT_MAX;
        int end = 0;
        for (auto bb: loop->get_loop()) {
            auto range = _block_id[bb];
            if (range.from < start) start = range.from;
            if (range.to > end) end = range.to;
        }
        _loop_id[loop] = BlockIDRange(start, end);
    }
}

void LinearScanSSA::buildIntervals() {
    for (auto bb: _BG.getInverseBBOrder()) {
        LiveData live;
        BlockIDRange bbRange = _block_id[bb];
        // Union all live in of successors
        for(auto succ: _cfg.getSuccBB(bb)) {
            live.unionLive(_live[succ]);
        }
        // Update live data
        for (auto op: live) {
            _interval[op].addRange(bbRange);
        }
        // Process operands of inst
        for (auto op: bb->get_reverse_instructions()) {
            // 保证指令是有返回值的，否则无需分配寄存器
            if (!op->is_void()) {
                // 我们的所有Op只有一个返回值，无需遍历
                _interval[op].setFrom(_inst_id[op]);
                live.erase(op);
            }
            // 没有返回值的也要考虑使用到的操作数。遍历来源操作数并设置Range，跳过常量、全局量、BB
            for (auto opd: op->get_operands()) {
                if (dynamic_cast<Instruction *>(opd) || dynamic_cast<Argument *>(opd)) {
                    _interval[opd].addRange(bbRange.from, _inst_id[op]);
                    live.add(opd);
                }
            }
        }
        // Process phi function
        for (auto inst: bb->get_instructions()) {
            if (dynamic_cast<PhiInst *>(inst)) {
                live.erase(inst);
            } else {
                // phi 只会出现在bb块开头
                break;
            }
        }
        // Process Loop
        if (auto loop = _lp.get_smallest_loop(bb)) {
            if (loop->get_loop_entry() == bb) {
                auto lrg = _loop_id[loop];
                for (auto op: live) {
                    _interval[op].addRange(bbRange.from, lrg.to);
                }
            }
        }
    }

}

void LinearScanSSA::linearScan() {
    // Step 1. 构造按Begin排序的Intervals
    std::vector<Interval> unhandled;
    std::vector<Interval> active;
    std::vector<Interval> inactive;
    std::vector<Interval> handled;
    for (auto i: _interval) {
        unhandled.push_back(i.second);
    }
    std::sort(unhandled.begin(), unhandled.end());
    while (!unhandled.empty()) {
        // 弹出队列中的第一个元素
        Interval current = unhandled.front();
        unhandled.erase(unhandled.begin());
        int position = current.getBegin();
        // check for intervals in active that are handled or inactive
        auto it = active.begin();
        while ( it != active.end()) {
            if ((*it).getEnd() < position) {
                handled.push_back(*it);
                it = active.erase(it);
            } else if (!it->cover(position)) {
                inactive.push_back(*it);
                it = active.erase(it);
            } else {
                it++;
            }
        }
        // check for intervals in inactive that are handled or active
        auto it2 = inactive.begin();
        while (it2 != inactive.end()) {
            if (it2->getEnd() < position) {
                handled.push_back(*it2);
                it2 = active.erase(it2);
            } else if (it2->cover(position)) {
                active.push_back(*it2);
                it2 = active.erase(it2);
            } else {
                it2++;
            }
        }
        // find a register for current

    }
}