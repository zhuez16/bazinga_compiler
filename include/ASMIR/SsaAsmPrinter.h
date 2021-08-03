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
        std::string ret = spacing + ".arch armv8 -a\n";
        ret += spacing + ".file \"test.sy\"\n";
        ret += spacing + ".data\n";
        // Print all global values
        for (auto gv: _builder->getGlobalValuables()) {
            ret += gv->print(_mapper);
        }
        ret += spacing + ".text\n";
        // Print all functions
        while (_builder->getFunctions().front()->getName() != "main"){
            _builder->getFunctions().insert(_builder->getFunctions().begin(),_builder->getFunctions().back());
            _builder->getFunctions().pop_back();
        }
        for (auto f:_builder->getFunctions()){
            ret += spacing + ".global "+f->getName()+"\n";
        }
        for (auto f: _builder->getFunctions()) {
            ret += f->print(_mapper);
            std::vector<ASInstruction *> phi_inst;
            int offset=f->get_pushed_offset();
            int arg_in_stack=f->getNumArguments();
            int i=0;
            for (auto instr:f->getBlockList().front()->getInstList()){
                if (i>=4){
                    instr->setOperand(0,ASConstant::getConstant(-4*(arg_in_stack-i+offset)));
                }
                i++;
            }
            for (auto b: f->getBlockList()) {
                b->addInstruction(b->print(_mapper));
                if (b==f->getBlockList().front()){
                    if (b->getInstList().size() < 4){
                        b->getInstList().clear();
                    }
                    else{
                        b->getInstList().pop_front();
                        b->getInstList().pop_front();
                        b->getInstList().pop_front();
                        b->getInstList().pop_front();
                    }
                }
                for (auto i: b->getInstList()) {
                    // If spill => Spill
                    if (i->getInstType()==ASInstruction::ASMPhiTy){
                        phi_inst.push_back(i);
                        continue;
                    }
                    int pos=_mapper->getInstructionID(i);
                    std::string inst="";
                    for (auto reg:_mapper->get_intervals()){
                        for (auto lr:reg.getIntervals()){
                            if (lr.second+1 == pos){
                                if (reg.getSpill() != -1){
                                    inst += "    str r"+std::to_string(reg.getRegister())+",[sp,#"+std::to_string(reg.getSpill())+"]\n";
                                }
                            }
                            if (lr.first == pos){
                                int get_spill=-1;
                                int last_pos=0;
                                for (auto reg_:_mapper->get_intervals()){
                                    if (reg_.getValue()==reg.getValue() && reg_.getEnd()<reg.getBegin() && reg_.getEnd()>last_pos){
                                        get_spill=reg_.getSpill();
                                        last_pos=reg_.getEnd();
                                    }
                                }
                                if (get_spill > 0)
                                    inst +="    ldr r"+std::to_string(reg.getRegister())+",[sp,#"+std::to_string(get_spill)+"]\n";
                            }
                        }
                    }
//                    ret += i->print(_mapper);
                    b->addInstruction(inst+i->print(_mapper));
                }
            }
            //generate mov for phi
            for (auto phi:phi_inst){
                auto instID=_mapper->getInstructionID(phi);
                for (auto val:phi->getOperands()){
                    auto bb=dynamic_cast<ASBlock *> (val);
                    auto temp_inst=bb->getInstList().back();
                    bb->getInstList().pop_back();
                    bb->addInstruction("    mov "+std::to_string(_mapper->getRegister(instID,phi))+","+std::to_string(_mapper->getRegister(instID,val))+"\n");
                    bb->addInstruction(temp_inst);
                }
            }
            for (auto b:f->getBlockList()){
                for (auto instr:b->get_asm_inst()){
                    ret+=instr;
                }
            }
            std::vector<int> saved_register;
            std::map<int, bool> saved_register_map;
            bool has_call=false;
            for (auto bb:f->getBlockList()){
                for (auto instr:bb->getInstList()){
                    if (instr->getInstType()==ASInstruction::ASMCallTy) has_call=true;
                    int reg=_mapper->getRegister(_mapper->getInstructionID(instr),instr);
                    if (!saved_register_map.count(reg)){
                        if (std::min(f->getNumArguments(),4) <= reg && reg < 11){
                            saved_register_map[reg]=true;
                            saved_register.push_back(reg);
                        }
                    }
                }
            }
            ret +="    add sp,r11,#0\n";
            ret +="    pop {";
            for (auto reg:saved_register){
                ret+="r"+std::to_string(reg)+",";
            }
            if (has_call) ret += "r11,pc}\n";
            else ret += "r11}\n    bx lr\n";
        }
        return ret;
    }
};


#endif //BAZINGA_COMPILER_SSAASMPRINTER_H
