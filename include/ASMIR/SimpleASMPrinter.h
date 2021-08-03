//
// Created by 顾超 on 2021/8/1.
// 简易的ASM IR遍历输出，无限寄存器，不考虑Spill
//

#ifndef BAZINGA_COMPILER_SIMPLEASMPRINTER_H
#define BAZINGA_COMPILER_SIMPLEASMPRINTER_H

#include "ASMIR/ASMBuilder.h"
#include "ASMIR/RegAllocMapper.h"

class SimpleASMPrinter {
private:
    ASMBuilder *_builder;
    RegMapper *_mapper;
public:
    SimpleASMPrinter(ASMBuilder *builder, RegMapper *mapper) : _builder(builder), _mapper(mapper) {
        // _mapper = new InfRegMapper();
    };

    ~SimpleASMPrinter() {  }

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
                    ret += i->print(_mapper);
                }
            }
        }
        return ret;
    }
};

#endif //BAZINGA_COMPILER_SIMPLEASMPRINTER_H
