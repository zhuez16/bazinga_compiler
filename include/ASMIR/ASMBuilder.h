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
    std::map<GlobalVariable *, ASGlobalValue *> _gv;

    ASFunction *_current_func;
    ASBlock *_current_block;

    void insertAndAddToMapping(Value *ori, ASInstruction *inst) {
        _mapping[ori] = inst;
        _current_block->addInstruction(inst);
    }

    ASInstruction *insert(ASInstruction *inst) {
        _current_block->addInstruction(inst);
        return inst;
    }

    void setInsertBlock(ASBlock *bb) {
        _current_block = bb;
        _current_func = bb->getFunction();
    }

    void setInsertBlock(BasicBlock *bb) {
        assert(_mapping.find(bb) != _mapping.end() && "Can't get ASMBlock by BasicBlock");
        _current_block = dynamic_cast<ASBlock *>(_mapping[bb]);
        assert(_current_block != nullptr && "ASMBlock Pointer is nullptr");
        _current_func = _current_block->getFunction();
    }

    template<class T> T* getMapping(Value *ori) {
        assert(_mapping.find(ori) != _mapping.end() && "Can't find mapping for value");
        return dynamic_cast<T *>(_mapping[ori]);
    }

    ASValue *getMapping(Value *ori) {
        if (auto c = dynamic_cast<ConstantInt *>(ori)) {
            return ASConstant::getConstant(c->get_value());
        }
        assert(_mapping.find(ori) != _mapping.end() && "Can't find mapping for value");
        return _mapping[ori];
    }

    ASFunction *getCurrentFunction() const {
        return _current_func;
    }

    ASGlobalValue *getGlobalValue(GlobalVariable *gv) {
        assert(_gv.find(gv) != _gv.end() && "Can't find mapping for global variable");
        return _gv[gv];
    }

    void setGlobalValue(GlobalVariable *ori, ASGlobalValue *ne) {
        _gv[ori] = ne;
        globals.push_back(ne);
    }
public:
    void build(Module *m);
};

#endif //BAZINGA_COMPILER_ASMBUILDER_H
