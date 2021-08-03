//
// Created by 顾超 on 2021/7/31.
//

#ifndef BAZINGA_COMPILER_REGALLOCMAPPER_H
#define BAZINGA_COMPILER_REGALLOCMAPPER_H

#include <string>
#include <map>
#include "codegen/LinearScanSSA.h"

class ASValue;
class ASInstruction;
class ASConstant;
class ASLabel;
class Interval;

class RegMapper {
public:
    virtual ~RegMapper() = default;
    virtual int getInstructionID(ASInstruction *inst) = 0;
    virtual int getRegister(int instID, ASValue *operand) = 0;
    virtual std::string getName(ASInstruction *inst, ASValue *operand);
    int getRegister(ASInstruction *inst, ASValue *operand) {
        return getRegister(getInstructionID(inst), operand);
    }
};

class InfRegMapper : public RegMapper {
private:
    std::map<ASValue *, int> _regMapper;
    int _currentRegIdx = 0;
public:
    int getInstructionID(ASInstruction *inst) final { return 0; }
    int getRegister(int instID, ASValue *operand) final {
        if (_regMapper.find(operand) == _regMapper.end()) {
            _regMapper[operand] = _currentRegIdx++;
        }
        return _regMapper[operand];
    }
};

class SsaRegMapper : public RegMapper{
private:
    std::map<ASInstruction *,int> inst_id;
    std::vector<Interval> intervals;
public:
    std::vector<Interval> get_intervals();

    SsaRegMapper(const std::map<ASInstruction *,int> &inst_id_, const std::vector<Interval> &interval_);
    int getInstructionID(ASInstruction *inst) final ;
    std::string getName(ASInstruction *instr, ASValue *val) final;
    int getRegister(int instID, ASValue *operand) final ;
};

#endif //BAZINGA_COMPILER_REGALLOCMAPPER_H
