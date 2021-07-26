//
// Created by mskhana on 2021/7/21.
//

#ifndef BAZINGA_COMPILER_INSTGEN_H
#define BAZINGA_COMPILER_INSTGEN_H

#ifndef SYSYC_INSTRUCTIONS_H
#define SYSYC_INSTRUCTIONS_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>


namespace InstGen {

const std::string tab = std::string(4, ' ');
const std::string newline = "\n";

const std::string reg_name[] = {"r0",  "r1", "r2", "r3", "r4",  "r5",
                                "r6",  "r7", "r8", "r9", "r10", "r11",
                                "r12", "sp", "lr", "pc"};

const int max_reg_id = 15;

const int imm_16_max = 65535;
const int imm_12_max = 4095;
const int imm_8_max = 255;

enum CmpOp {
  EQ, // ==
  NE, // !=
  GT, // >
  GE, // >=
  LT, // <
  LE, // <=
  NOP
};

class RegValue {
public:
  virtual bool is_reg() const = 0;
  virtual bool is_constant() const = 0;
  virtual bool has_shift() const = 0;
  virtual std::string getName() const = 0;
};

class Reg : public RegValue {
  int id;

public:
  explicit Reg(int id) : id(id) {
    if (id < 0 || id > max_reg_id) {
      std::cerr << "Invalid Reg ID!" << std::endl;
      abort();
    }
  }
  bool is_reg() const { return true; }
  bool is_constant() const { return false; }
  bool has_shift() const { return false; }
  int getID() const { return this->id; }
  std::string getName() const { return reg_name[id]; }
  const bool operator<(const Reg &rhs) const { return this->id < rhs.id; }
  const bool operator==(const Reg &rhs) const { return this->id == rhs.id; }
  const bool operator!=(const Reg &rhs) const { return this->id != rhs.id; }
};

class RegShift : public RegValue {
public:
  enum ShiftType { lsl, lsr, asl, asr };

private:
  int id;
  int shift;
  ShiftType _t;

public:
  explicit RegShift(int id, int shift, ShiftType _t = ShiftType::lsl)
      : id(id), shift(shift), _t(_t) {
    if (id < 0 || id > max_reg_id) {
      std::cerr << "Invalid Reg ID!" << std::endl;
      abort();
    }
    if (shift < 0 || shift > 31) {
      std::cerr << "Invalid Reg shift!" << std::endl;
      abort();
    }
  }
  bool is_reg() const { return true; }
  bool is_constant() const { return false; }
  bool has_shift() const { return true; }
  int getID() const { return this->id; }
  int getShift() const { return this->shift; }
  std::string getName() const {
    std::string shift_str;
    switch (this->_t) {
    case ShiftType::lsl:
      shift_str = "lsl";
      break;
    case ShiftType::asl:
      shift_str = "asl";
      break;
    case ShiftType::lsr:
      shift_str = "lsr";
      break;
    case ShiftType::asr:
      shift_str = "asr";
      break;
    default:
      break;
    }
    return reg_name[id] + ", " + shift_str + " " + "#" +
           std::to_string(this->getShift());
  }
  const bool operator<(const RegShift &rhs) const { return this->id < rhs.id; }
  const bool operator==(const RegShift &rhs) const {
    return this->id == rhs.id;
  }
  const bool operator!=(const RegShift &rhs) const {
    return this->id != rhs.id;
  }
};

const Reg sp = Reg(13);
const Reg lr = Reg(14);
const Reg pc = Reg(15);

class Addr {
  Reg reg;
  int offset;

public:
  explicit Addr(Reg reg, int offset) : reg(reg), offset(offset) {}
  Reg getReg() const { return this->reg; }
  int getOffset() const { return this->offset; }
  std::string getName() const {
    return "[" + reg.getName() + ", " + "#" + std::to_string(this->offset) +
           "]";
  }
};

class Constant : public RegValue {
  int RegValue;

public:
  explicit Constant(int RegValue) : RegValue(RegValue) {}
  bool is_reg() const { return false; }
  bool is_constant() const { return true; }
  bool has_shift() const { return false; }
  int getRegValue() const { return this->RegValue; }
  std::string getName() const { return "#" + std::to_string(this->RegValue); }
};

class Label {
  std::string label;
  int offset;

public:
  explicit Label(std::string label, int offset)
      : label(label), offset(offset) {}
  explicit Label(std::string label) : label(label), offset(0) {}
  std::string getName() const { return label + "+" + std::to_string(offset); }
};

std::string condCode(const CmpOp &cond);
std::string gen_push(const std::vector<Reg> &reg_list);
std::string gen_pop(const std::vector<Reg> &reg_list);
std::string gen_mov(const Reg &target, const RegValue &source, const CmpOp &cond = NOP);
std::string gen_mvn(const Reg &target, const RegValue &source, const CmpOp &cond = NOP);
std::string gen_movw(const Reg &target, const RegValue &source, const CmpOp &cond = NOP);
std::string gen_movt(const Reg &target, const RegValue &source, const CmpOp &cond = NOP);
std::string setRegValue(const Reg &target, const Constant &source);
std::string gen_adrl(const Reg &target, const Label &source);
std::string gen_ldr(const Reg &target, const Addr &source);
std::string gen_ldr(const Reg &target, const Label &source);
std::string gen_ldr(const Reg &target, const Reg &base, const Reg &offset);
std::string gen_ldr(const Reg &target, const Reg &base, const Reg &offset,
                const Constant &shift);
std::string gen_str(const Reg &source, const Addr &target);
std::string gen_str(const Reg &source, const Label &target);
std::string gen_str(const Reg &target, const Reg &base, const Reg &offset);
std::string gen_str(const Reg &target, const Reg &base, const Reg &offset,
                const Constant &shift);
std::string gen_bl(const std::string &target_func_name);
std::string gen_add(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_sub(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_rsb(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_and(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_orr(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_eor(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_clz(const Reg &target, const Reg &op1);
std::string gen_lsl(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_asl(const Reg &target, const Reg &op1,
                const RegValue &op2); // same as lsl
std::string gen_lsr(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_asr(const Reg &target, const Reg &op1, const RegValue &op2);
std::string gen_mul(const Reg &target, const Reg &op1, const Reg &op2);
std::string gen_smmul(const Reg &target, const Reg &op1, const Reg &op2);
std::string gen_mla(const Reg &target, const Reg &op1, const Reg &op2,
                const Reg &op3);
std::string gen_smmla(const Reg &target, const Reg &op1, const Reg &op2,
                  const Reg &op3);
std::string gen_mls(const Reg &target, const Reg &op1, const Reg &op2,
                const Reg &op3);
std::string gen_smull(const Reg &target, const Reg &op1, const Reg &op2,
                  const Reg &op3);
std::string gen_sdiv(const Reg &target, const Reg &op1, const Reg &op2);
std::string gen_cmp(const Reg &lhs, const RegValue &rhs);
std::string gen_b(const Label &target, const CmpOp &op = NOP);
std::string instConst(std::string (*inst)(const Reg &target, const Reg &op1,
                                          const RegValue &op2),
                      const Reg &target, const Reg &op1, const Constant &op2);
std::string instConst(std::string (*inst)(const Reg &op1, const RegValue &op2),
                      const Reg &op1, const Constant &op2);
std::string gen_load(const Reg &target, const Addr &source);
std::string gen_store(const Reg &source, const Addr &target);
std::string gen_swi(const Constant &id);
std::string gen_bic(const Reg &target, const Reg &op1, const RegValue &op2);

//std::string bic(const Reg &target, const Reg &v1, const Reg &v2, const Reg &v3);
std::tuple<int, int, int> choose_multiplier(int d, int N);
std::string divConst(const Reg &target, const Reg &source,
                     const Constant &divisor);

}; // namespace InstGen

const InstGen::Reg vinst_temp_reg = InstGen::Reg(11);


#endif //BAZINGA_COMPILER_INSTGEN_H
