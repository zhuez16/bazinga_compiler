//
// Created by 顾超 on 2021/7/26.
//

#ifndef BAZINGA_COMPILER_MEMANALYSIS_H
#define BAZINGA_COMPILER_MEMANALYSIS_H

#include "analysis_pass_manager.h"

class MemAccess : public AAPass {
public:
    MemAccess(Module *m) : AAPass(100, m) {}

    void run() final;

    /**
     * 判断函数是否不包含内存访问
     * 内存访问包括读全局变量 与 读参数中的数组变量
     * 不包括对静态存储的访问
     * @param f
     * @return
     */
    bool isNoMemoryAccess(Function *f);
    bool isOnlyReadAccess(Function *f);
    bool isMemoryRWAccess(Function *f);
    bool isNoMemoryAccess(CallInst *call) { return isNoMemoryAccess(dynamic_cast<Function *>(call->get_operand(0))); }
    bool isOnlyReadAccess(CallInst *call) { return isOnlyReadAccess(dynamic_cast<Function *>(call->get_operand(0))); }
    bool isMemoryRWAccess(CallInst *call) { return isMemoryRWAccess(dynamic_cast<Function *>(call->get_operand(0))); }



};

#endif //BAZINGA_COMPILER_MEMANALYSIS_H
