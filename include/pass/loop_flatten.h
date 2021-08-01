//
// Created by 顾超 on 2021/7/27.
//

#ifndef BAZINGA_COMPILER_LOOP_FLATTEN_H
#define BAZINGA_COMPILER_LOOP_FLATTEN_H
#include "pass_manager.h"
#include "loop_search.h"
#include "dominator.h"

/**
 * 嵌套循环展开
 * Reference: llvm::transforms::scalar::LoopFlatten
 */
class LoopFlatten : public Pass {
private:
    dominator *dom;
    LoopSearch *lp;

public:
    LoopFlatten(Module *m) : Pass(m), dom(new dominator(m)), lp(new LoopSearch(m)) {}

    void run() final;

    bool flatten(Function *f);

    bool flatten_loop_pair(Loop *outer, Loop *inner);

    bool find_components(Loop *loop, std::set<Instruction *> &iterInstructions,
                         PhiInst *&phi,
                         Value *&limit,
                         BinaryInst *&increment,
                         BranchInst *&branch);
};

#endif //BAZINGA_COMPILER_LOOP_FLATTEN_H
