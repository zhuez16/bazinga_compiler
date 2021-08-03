//
// Created by mskhana on 2021/8/2.
//

#ifndef BAZINGA_COMPILER_SSAASMPRINTER_H
#define BAZINGA_COMPILER_SSAASMPRINTER_H


#include "ASMIR/ASMBuilder.h"
#include "ASMIR/RegAllocMapper.h"

class SsaASMPrinter {
private:
    ASMBuilder *_builder;
    SsaRegMapper *_mapper;
public:
    SsaASMPrinter(ASMBuilder *builder, SsaRegMapper *mapper) : _builder(builder), _mapper(mapper) {
        // _mapper = new InfRegMapper();
    };

    ~SsaASMPrinter() {  }

    std::string print() {
        const std::string spacing = "    ";
        std::string ret = spacing + ".arch armv7\n";
        ret += spacing + ".file \"test.sy\"";
        ret += spacing + ".data\n";
        // Print all global values
        for (auto gv: _builder->getGlobalValuables()) {
            ret += gv->print(_mapper);
        }
        ret += spacing + ".text\n";
        // Print all functions
        for (auto f: _builder->getFunctions()) {
            ret += f->print(_mapper);
            for (auto b: f->getBlockList()) {
                ret += b->print(_mapper);
                for (auto i: b->getInstList()) {
                    // If spill => Spill
                    int pos=_mapper->getInstructionID(i);
                    for (auto reg:_mapper->get_intervals()){
                        for (auto lr:reg.getIntervals()){
                            if (lr.second+1 == pos){
                                if (reg.getSpill() != -1){
                                    ret+="    str "+std::to_string(reg.getRegister())+",[sp,"+std::to_string(reg.getSpill())+"]\n";
                                }
                            }
                            else if (lr.first == pos){
                                int get_spill=-1;
                                int last_pos=0;
                                for (auto reg_:_mapper->get_intervals()){
                                    if (reg_.getValue()==reg.getValue() && reg_.getEnd()<reg.getBegin() && reg_.getEnd()>last_pos){
                                        get_spill=reg_.getSpill();
                                        last_pos=reg_.getEnd();
                                    }
                                }
                                if (get_spill > 0)
                                    ret +="    load"+std::to_string(reg.getRegister())+",[sp,"+std::to_string(get_spill)+"]\n";
                            }
                        }
                    }
                    ret += i->print(_mapper);
                }
            }
        }
        return ret;
    }
};


#endif //BAZINGA_COMPILER_SSAASMPRINTER_H
