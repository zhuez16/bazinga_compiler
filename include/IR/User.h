#ifndef SYSYC_USER_H
#define SYSYC_USER_H

#include "Value.h"
#include <vector>
// #include <memory>
// class Value;
class User : public Value {
public:
  User(Type *type, const std::string &name = "", unsigned num_ops = 0);
  ~User() = default;

  std::vector<Value *> &get_operand_list();

  // start from 0
  Value *get_operand(unsigned i) const;

  // start from 0, auto add use
  void set_operand(unsigned i, Value *v);

  void add_operand(Value *v);
  void remove_operand(unsigned i);
  void remove_operand(unsigned i, unsigned j);

  unsigned get_operand_num() const;

  // TODO(zyh) 待确认
  //std::vector<Value *> &getOperands() { return operands_; }

  //virtual void print() override {}

  // remove the use of all operands
  void remove_useof_operand();

  void set_num(unsigned num) {
    num_ops_ = num;
    operands_.resize(num, nullptr);
  }
  void clear_operand() {
    num_ops_ = 0;
    remove_useof_operand();
    operands_.clear();
  }

private:
  // std::unique_ptr< std::list<Value *> > operands_;   // operands of this
  // value
  std::vector<Value *> operands_; // operands of this value
  unsigned num_ops_;
};

#endif // SYSYC_USER_H