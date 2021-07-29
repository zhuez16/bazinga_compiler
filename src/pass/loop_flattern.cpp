//
// Created by 顾超 on 2021/7/27.
//

#include "pass/loop_flatten.h"

/**
 * 从循环中获得循环关键信息
 * @param loop
 * @param iterInstructions
 * @param phi
 * @param limit
 * @param increment
 * @param branch
 * @return
 */
bool find_components(Loop *loop, std::set<Instruction *> &iterInstructions, PhiInst *&phi, Value *&limit,
                     BinaryInst *&increment, BranchInst *&branch) {
    BasicBlock *latch = loop->getLoopLatch();
    if (loop->getExitingBB() != latch) {
        std::cout << "FindComponentFailed: Loop got latch != exiting" <<std::endl;
        return false;
    }
    branch = dynamic_cast<BranchInst *>(latch->get_terminator());
    if (branch == nullptr || !branch->is_cond_br()) {
        std::cout << "FindComponentFailed: Can't find condition branch or got infinity loop." <<std::endl;
        return false;
    }
    iterInstructions.insert(branch);
    // 标记了是否会在True时退出循环
    bool continueOnTrue = loop->contain_bb(branch->getTrueBB());

}

bool LoopFlatten::flatten_loop_pair(Loop *outer, Loop *inner) {

}

bool LoopFlatten::flatten(Function *f) {
    for (auto loop: lp->get_loop(f)) {
        auto outer = lp->get_outer_loop(loop);
        if (outer == nullptr) { continue; }

    }
}