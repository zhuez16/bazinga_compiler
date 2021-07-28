//
// Created by 顾超 on 2021/7/24.
//

#ifndef BAZINGA_COMPILER_SINK_H
#define BAZINGA_COMPILER_SINK_H

#include "pass_manager.h"
#include "CFG.h"
#include "dominator.h"
#include "loop_search.h"
#include "IR/Instruction.h"

class CodeSinking: public Pass {

public:
    explicit CodeSinking(Module *m) : Pass(m), cfg(new CFG(m)), dom(new dominator(m)), lp(new LoopSearch(m)) {}

    void run() override;

private:

    CFG *cfg;

    dominator *dom;

    LoopSearch *lp;

    bool SinkInstruction(Instruction *i);

    bool processBB(BasicBlock *bb);

    bool iterSinkInstruction(Function *f);

    bool IsAcceptableTarget(Instruction *Inst, BasicBlock *SuccToSinkTo);
};

#endif //BAZINGA_COMPILER_SINK_H
