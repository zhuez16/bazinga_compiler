//
// Created by 顾超 on 2021/7/30.
//

#ifndef BAZINGA_COMPILER_ASMBUILDER_H
#define BAZINGA_COMPILER_ASMBUILDER_H

#include "ASMIR/ASValue.hpp"
#include "IR/Module.h"

class ASMBuilder {
private:
    std::vector<ASGlobalValue *> globals;
    std::vector<ASFunction *> functions;
    std::map<Value *, ASValue *> _mapping;

    ASFunction *_current_func;
    ASBlock *_current_block;

    void insertAndAddToMapping(Value *ori, ASInstruction *inst) {
        _mapping[ori] = inst;
        _current_block->addInstruction(inst);
    }

    void insert(ASInstruction *inst) {
        _current_block->addInstruction(inst);
    }

    void setInsertBlock(ASBlock *bb) {
        _current_block = bb;
    }

    void setInsertBlock(BasicBlock *bb) {
        assert(_mapping.find(bb) != _mapping.end() && "Can't get ASMBlock by BasicBlock");
        _current_block = dynamic_cast<ASBlock *>(_mapping[bb]);
        assert(_current_block != nullptr && "ASMBlock Pointer is nullptr");
        _current_func = _current_block->getFunction();
    }
public:
    void build(Module *m);
};

#endif //BAZINGA_COMPILER_ASMBUILDER_H
