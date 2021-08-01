//
// Created by 顾超 on 2021/7/31.
//

#ifndef BAZINGA_COMPILER_REGALLOCMAPPER_H
#define BAZINGA_COMPILER_REGALLOCMAPPER_H

#include <string>

class ASValue;
class ASInstruction;

class RegMapper {
public:
    virtual int getInstructionID(ASInstruction *inst) = 0;
    virtual int getRegister(int instID, ASValue *operand) = 0;
    virtual std::string getName(ASInstruction *inst, ASValue *operand);
    int getRegister(ASInstruction *inst, ASValue *operand) {
        return getRegister(getInstructionID(inst), operand);
    }
};

#endif //BAZINGA_COMPILER_REGALLOCMAPPER_H
