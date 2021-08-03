//
// Created by 顾超 on 2021/7/29.
//

#include "codegen/LinearScanSSA.h"
#include <vector>
#include <algorithm>
#include <climits>


void LinearScanSSA::run(ASMBuilder *builder, Module *m) {
    {
        map = builder->getValueMap();
        delete _cfg;
        delete _lp;
        delete BG;
        unhandled.clear();
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
            }
        }
    }
}

std::string Interval::toString(RegMapper *mapper) const {
    {
        std::string ret = mapper->getName(nullptr, _v);
        ret += "  Reg: " + std::to_string(_reg);
        ret += "  Spi: " + std::to_string(_spill);
        ret += "  Range: ";
        for (auto iv: _intervals) {
            ret += "[" + std::to_string(iv.first) + " - " + std::to_string(iv.second) + "] ";
        }
        if (_fixed) {
            ret += "  [FIXED]";
        }
        ret += "\n";
        return ret;
    }

}

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

void LinearScanSSA::assignOpID(Function *of, ASFunction *f) {
    _interval.clear();
    int id = 1;
    // 加入init块访问
    auto init_bb = f->getBlockList().front();
    // 保证只有mov和load
    for (auto inst: init_bb->getInstList()) {
        _inst_id[inst] = id;
        _interval[inst] = Interval(inst, id);
        ++id;
    }
    for (auto bb: BG->getASMBBOrder(map)) {
        int begin = id;
        for (auto inst: bb->getInstList()) {
            _inst_id[inst] = id;
            if (inst->hasResult()) {
                _interval[inst] = Interval(inst, id);
            }
            if (inst->getInstType() == ASInstruction::ASMCallTy) {
                auto callee = dynamic_cast<ASFunction *>(inst->getOperand(0));
                for (int i = 0; i < std::min(callee->getNumArguments(), 4); ++i) {
                    Interval iv(id, i, inst, true);
                    fixed.push_back(iv);
                }
            }
            ++id;
        }
        int end = id - 1;
        _block_id[bb] = BlockIDRange(begin, end);
    }
    for (auto loop: _lp->get_loop(of)) {
        int start = INT_MAX;
        int end = 0;
        for (auto bb: loop->get_loop()) {
            auto range = _block_id[dynamic_cast<ASBlock *>(map[bb])];
            if (range.from < start) start = range.from;
            if (range.to > end) end = range.to;
        }
        _loop_id[loop] = BlockIDRange(start, end);
    }
}

void LinearScanSSA::buildIntervals() {
    auto bb_l = BG->getInverseBBOrder();
    bb_l.push_back(nullptr);
    for (auto bb: bb_l) {
        LiveData live;
        auto ASBB = (bb) ? dynamic_cast<ASBlock *>(map[bb]) : currentFunc->getBlockList().front();
        assert(ASBB && "Can't get ASM Block.");
        BlockIDRange bbRange = _block_id[ASBB];
        // Union all live in of successors
        if (bb == nullptr) {
            live.unionLive(_live[dynamic_cast<ASBlock *>(map[_cfg->getEntryBB()])]);
        }
        for(auto succ: _cfg->getSuccBB(bb)) {
            live.unionLive(_live[dynamic_cast<ASBlock *>(succ)]);
        }
        // Update live data
        for (auto op: live) {
            _interval[op].addRange(bbRange);
        }
        // Process operands of inst
        for (auto op: ASBB->getReverseInstList()) {
            // 保证指令是有返回值的，否则无需分配寄存器
            if (op->hasResult()) {
                // 我们的所有Op只有一个返回值，无需遍历
                _interval[op].setFrom(_inst_id[op]);
                live.erase(op);
            }
            // 没有返回值的也要考虑使用到的操作数。遍历来源操作数并设置Range，跳过常量、全局量、BB
            for (auto opd: op->getOperands()) {
                if (dynamic_cast<ASInstruction *>(opd) || dynamic_cast<ASArgument *>(opd)) {
                    _interval[opd].addRange(bbRange.from, _inst_id[op]);
                    live.add(opd);
                }
            }
        }
        // Process phi function
        for (auto inst: ASBB->getInstList()) {
            if (dynamic_cast<ASPhiInst *>(inst)) {
                live.erase(inst);
            } else {
                // phi 只会出现在bb块开头
                break;
            }
        }
        // Process Loop
        if (bb) {
            if (auto loop = _lp->get_smallest_loop(bb)) {
                if (loop->get_loop_entry() == bb) {
                    auto lrg = _loop_id[loop];
                    for (auto op: live) {
                        _interval[op].addRange(bbRange.from, lrg.to);
                    }
                }
            }
        }
    }

}


void Interval::addRange(int from, int to){
    if (_end == -1) {
        _intervals.clear();
        _intervals.emplace_back(from, to);
        _begin = from;
        _end = to;
        return;
    }
    std::pair<int,int> temp=std::make_pair(from,to);
    this->_begin = std::min(this->_begin, from);
    this->_end = std::max(this->_end, to);
    for (auto it = _intervals.begin(); it != _intervals.end();){
        if (temp.second+1<it->first){
            ++it;
            break;
        }
        else if (temp.first>it->second+1){
            ++it;
            continue;
        }
        else{
            temp.first=std::min(temp.first,it->first);
            temp.second=std::max(temp.second,it->second);
            it = _intervals.erase(it);
        }
    }
    this->_intervals.push_back(temp);
    std::sort(this->_intervals.begin(),this->_intervals.end());
}

void LinearScanSSA::linearScan() {
    // 清空所有缓存
    unhandled.clear();
    // handled.clear();
    active.clear();
    inactive.clear();
    // Step 1. 构造按Begin排序的Intervals
    for (const auto& i: _interval) {
        // 若是函数调用返回则固定到0号寄存器
        if (dynamic_cast<ASInstruction *>(i.first)->getInstType() == ASInstruction::ASMCallTy) {
            auto a = i.second;
            a.setRegister(0);
            unhandled.push_back(a);
        } else {
            unhandled.push_back(i.second);
        }
    }
    // 将函数的Fixed也传进去
    for (const auto& f: fixed) {
        active.push_back(f);
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
                it2 = inactive.erase(it2);
            } else if (it2->cover(position)) {
                active.push_back(*it2);
                it2 = inactive.erase(it2);
            } else {
                it2++;
            }
        }
        // find a register for current
        if (!tryAllocateFreeRegister(current, current.getBegin())) {
            // Fail to find a empty register to allocate
            allocateBlockedRegister(current, current.getBegin());
        }
        if (current.getRegister() != -1) {
            active.push_back(current);
        }
    }
    // move every thing in active and inactive to handled and finish algo
    handled.insert(handled.end(), active.begin(), active.end());
    handled.insert(handled.end(), inactive.begin(), inactive.end());
}

bool LinearScanSSA::tryAllocateFreeRegister(Interval &current, int position) {
    int freeUntilPosition[NUM_REG];
    // set freeUntilPos of all physical registers to maxInt
    for (int & i : freeUntilPosition) {
        i = INT_MAX;
    }
    // active and inactive
    for (const auto& it: active) {
        freeUntilPosition[it.getRegister()] = 0;
    }
    for (auto it: inactive) {
        // TODO
        if (int nextPos = it.intersect(position, current)) {
            if (nextPos > 0)
                freeUntilPosition[it.getRegister()] = std::min(nextPos, freeUntilPosition[it.getRegister()]);
        }
    }
    // 如果一个Interval已经分配了寄存器，我们认为他被指派了固定寄存器编号
    // reg = register with highest freeUntilPos
    int max_idx = 0;
    if (current.getRegister() != -1) {
        max_idx = current.getRegister();
    } else {
        for (int i = 1; i < NUM_REG; ++i) {
            if (freeUntilPosition[i] > freeUntilPosition[max_idx]) max_idx = i;
        }
    }
    if (freeUntilPosition[max_idx] == 0) {
        // 没有空闲寄存器，分配失败
        return false;
    }
    else if (current.getEnd() < freeUntilPosition[max_idx]) {
        // 寄存器在Live Hole中完全可用
        current.setRegister(max_idx);
        return true;
    } else {
        // 寄存器在Live Hole中部分可用
        unhandled.push_back(current.split(freeUntilPosition[max_idx]));
        // TODO: need to re sort unhandled ?
        std::sort(unhandled.begin(), unhandled.end());
        current.setRegister(max_idx);
        return true;
    }

}

void LinearScanSSA::allocateBlockedRegister(Interval &current, int position) {
    int nextUsePos[NUM_REG];
    Interval nextList[NUM_REG];
    // set freeUntilPos of all physical registers to maxInt
    for (int & nextUsePo : nextUsePos) {
        nextUsePo = INT_MAX;
    }
    // active and inactive
    for (const auto& it: active) {
        if (it.intersect(current)) {
            auto np = it.getNextUse(position);
            if (np < nextUsePos[it.getRegister()]) {
                nextUsePos[it.getRegister()] = np;
                nextList[it.getRegister()] = it;
            }
        }
    }
    for (const auto& it: inactive) {
        if (it.intersect(current)) {
            if (it.intersect(current)) {
                auto np = it.getNextUse(position);
                if (np < nextUsePos[it.getRegister()]) {
                    nextUsePos[it.getRegister()] = np;
                    nextList[it.getRegister()] = it;
                }
            }
        }
    }
    // reg = register with highest freeUntilPos
    int max_idx = 0;
    if (current.getRegister() != -1) {
        max_idx = current.getRegister();
    } else {
        for (int i = 1; i < NUM_REG; ++i) {
            if (nextList[max_idx].isFixed() || (!nextList[i].isFixed() && nextUsePos[i] > nextUsePos[max_idx])) max_idx = i;
        }
    }
    int nextUse = current.getNextUse(position);
    if (current.getRegister() == -1 && nextUse > nextUsePos[max_idx]) {
        // all other intervals are used before current, so it is best to spill current itself
        int spillId = requireNewSpillSlot(current.getValue());
        current.setSpill(spillId);
        unhandled.push_back(current.split(nextUse));
        std::sort(unhandled.begin(), unhandled.end());
    } else {
        bool flag = true;
        // make sure that current does not intersect with the fixed interval for reg
        /*
        for(const auto& it : fixed){
            if (it.intersect(current))
                if(it.getRegister()==max_idx)
                {
                    int spillId = requireNewSpillSlot(current.getValue());
                    auto spt = current.split(position);
                    current.setSpill(spillId);
                    unhandled.push_back(spt);
                    std::sort(unhandled.begin(), unhandled.end());
                    return;
                }
        }
         */
        // 找出当前占用该寄存器的Interval
        Interval to_spill;
        auto it = active.begin();
        while ( it != active.end()) {
            if ((*it).getRegister()==max_idx) {
                flag = false;
                to_spill = (*it);
                active.erase(it);
                break;
            }
            it++;
        }
        if(flag)
        {
            it = inactive.begin();
            while ( it != inactive.end()) {
                if ((*it).getRegister()==max_idx) {
                    to_spill = (*it);
                    inactive.erase(it);
                    break;
                }
                it++;
            }
        }
        // 为Spill分配栈空间
        int spillId = requireNewSpillSlot(to_spill.getValue());
        to_spill.setSpill(spillId);
        // 如果是因为固定寄存器导致Spill，位置设置为当前位置
        if (current.getRegister() == -1) {
            unhandled.push_back(to_spill.split(nextUsePos[max_idx]));
            handled.push_back(to_spill);
        } else {
            // 寄存器同时被fixed而且被指定，摆烂
            if (!to_spill.isFixed()) {
                unhandled.push_back(to_spill.split(position));
                handled.push_back(to_spill);
            }
        }

        // 为当前Interval分配指定寄存器
        current.setRegister(max_idx);
        std::sort(unhandled.begin(), unhandled.end());
    }
    /*
    // make sure that current does not intersect with the fixed interval for reg
    for(auto it : fixed){
        if (int nextPos = it.intersect(position, current))
            if (nextPos > 0)
                if(it.getRegister()==max_id)
                {
                    int spillId = requireNewSpillSlot();
                    current.setSpill(spillId);
                    unhandled.push_back(current.split(nextUse));
                    std::sort(unhandled.begin(), unhandled.end());
                }
    }
    */
}

int LinearScanSSA::requireNewSpillSlot(ASValue *v) {
    { return currentFunc->allocStack(v, 1); }
}
