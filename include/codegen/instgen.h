//
// Created by mskhana on 2021/7/21.
//

#ifndef BAZINGA_COMPILER_INSTGEN_H
#define BAZINGA_COMPILER_INSTGEN_H
#include <string>
const int max_reg_id=15;
const std::string reg_name[]={
        "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12",
        "sp","lr","pc"
};
const std::string newline="\n";
const std::string tab="    ";
class instValue {
public:
    virtual bool is_reg() const=0;
    virtual bool is_const() const=0;
    virtual bool has_shift() const=0;
    virtual std::string get_name() const=0;
};

class Reg: public instValue{
    int reg_id;
public:
    explicit Reg(int id): reg_id(id) {}
    bool is_reg() const{return true;}
    bool is_const() const{return false;}
    bool has_shift() const{return false;}
    int get_id() const{return this->reg_id;}
    std::string get_name() const{return reg_name[reg_id];}
    const bool operator ==(const Reg &other) const{return this->reg_id == other.reg_id;}
    const bool operator !=(const Reg &other) const{return this->reg_id != other.reg_id;}
};

class Const: public instValue{
    int const_num;
public:
    explicit Const(int num): const_num(num);
    bool is_reg() const{return false;}
    bool is_const() const{return true;}
    bool has_shift() const{return false;}
    int get_value() const{return this->const_num;}
    std::string get_name() const{return "#"+std::to_string(this->const_num);}
};

class Addr{
    Reg reg;
    int offset;
public:
    explicit Addr(Reg reg, int offset): reg(reg),offset(offset){}
    Reg get_reg() const {return this->reg;}
    int get_offset() const{return this->offset;}
    std::string get_name() const{
        return "["+reg.get_name()+",#"+std::to_string(this->offset)+"]";
    }

};
class Label{
    std::string label;
    int offset;

public:
    explicit Label(std::string label,int offset):label(label),offset(offset){}
    std::string get_name() const {return label+"+"+std::to_string(offset);}
};
enum cmpOperand{
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE,
    NOP
};
std::string cond_code(const cmpOperand op);
std::string gen_add(const Reg &target, const Reg &op1, const instValue &op2);
std::string gen_mul(const Reg &target, const Reg &op1, const instValue &op2);
std::string gen_sub(const Reg &target, const Reg &op1, const instValue &op2);
std::string gen_div(const Reg &target, const Reg &op1, const instValue &op2);
std::string gen_mov(const Reg &target, const Reg &op1);
std::string gen_set_value(const Reg &target, const Reg &source);
std::string gen_get_addr(const Reg &target, const Reg &source);
std::string gen_asr(const Reg &target, const Reg &op1, const instValue &op2);
std::string gen_lsl(const Reg &target, const Reg &op1, const instValue &op2);
#endif //BAZINGA_COMPILER_INSTGEN_H
