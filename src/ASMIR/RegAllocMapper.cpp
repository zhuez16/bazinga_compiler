//
// Created by 顾超 on 2021/8/1.
//
#include "ASMIR/RegAllocMapper.h"
#include "ASMIR/ASValue.hpp"


std::string RegMapper::getName(ASInstruction *inst, ASValue *operand) {
    if (dynamic_cast<ASConstant *>(operand)) {
        return operand->print(this);
    }
    else if (dynamic_cast<ASLabel *>(operand)) {
        return operand->getName();
    }
    else {
        return " r" + std::to_string(getRegister(inst, operand));
    }
}
