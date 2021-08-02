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

int SsaRegMapper::getInstructionID(ASInstruction *inst) {
    return inst_id.at(inst);
}

std::string SsaRegMapper::getName(ASInstruction *instr, ASValue *val) {
    int pos=inst_id.at(instr);
    if (dynamic_cast<ASConstant *>(val)) {
        return val->print(this);
    }
    else if (dynamic_cast<ASLabel *>(val)) {
        return val->getName();
    }
    for (auto interval:intervals){
        if (interval.getValue() == val){
            for (auto temp:interval.getIntervals()){
                if (pos>=temp.first && pos<=temp.second) return " R"+std::to_string(interval.getRegister());
            }
        }
    }
    assert("A allocated space don't have a register!");
}

int SsaRegMapper::getRegister(int instID, ASValue *operand){
    for (auto interval:intervals){
        if (interval.getValue() == operand){
            for (auto temp:interval.getIntervals()){
                if (instID>=temp.first && instID<=temp.second) return interval.getRegister();
            }
        }
    }
}

SsaRegMapper::SsaRegMapper(const std::map<ASInstruction *, int> &inst_id_, const std::vector<Interval> &interval_)
        :inst_id(inst_id_),intervals(interval_){}

std::vector<Interval> SsaRegMapper::get_intervals() {return intervals;}
