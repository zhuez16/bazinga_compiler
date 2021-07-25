//
// Created by 顾超 on 2021/7/25.
//

#ifndef BAZINGA_COMPILER_CODEELIMINATION_H
#define BAZINGA_COMPILER_CODEELIMINATION_H

#include "pass.h"
#include "active_vars.h"


/***===
 * 删除块中不活跃的指令。使用活跃变量分析方法
 */
class CodeElimination : public Pass {
public:
    CodeElimination(Module *m) : Pass(m), act(new active_vars(m)) {}

    void run() final;

private:
    active_vars *act;
};

#endif //BAZINGA_COMPILER_CODEELIMINATION_H
