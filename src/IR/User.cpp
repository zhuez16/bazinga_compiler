#include "User.h"
//#include "ReturnVal.h"
#include <cassert>

User::User(Type *ty, const std::string &name, unsigned num_ops)
    : Value(ty, name), num_ops_(num_ops) {
  // if (num_ops_ > 0)
  //   operands_.reset(new std::list<Value *>());
  operands_.resize(num_ops_, nullptr);
}

std::vector<Value *> &User::get_operand_list() { return operands_; }

Value *User::get_operand(unsigned i) const { return operands_[i]; }

void User::set_operand(unsigned i, Value *v) {
    /*
  exit_ifnot(_OutOfIndex_setOperand_User, i < num_ops_ && i >= 0 &&
                                              i < operands_.size() &&
                                              "setOperand out of index");
  // assert(operands_[i] == nullptr && "ith operand is not null");
  */
  operands_[i] = v;
  v->add_use(this, i);
}

unsigned User::get_operand_num() const { return num_ops_; }

void User::add_operand(Value *v) {
  operands_.push_back(v);
  v->add_use(this, num_ops_);
  num_ops_++;
}

void User::remove_operand(unsigned i) {
  remove_useof_operand();
  num_ops_ = 0;
  std::vector<Value *> item;
  item.swap(operands_);
  operands_.clear();
  for (int j = 0; j < item.size(); j++) {
    if (i == j) {
      continue;
    } else {
      add_operand(item[j]);
    }
  }
}

void User::remove_operand(unsigned i, unsigned j) {
  remove_useof_operand();
  num_ops_ = 0;
  std::vector<Value *> item;
  item.swap(operands_);
  operands_.clear();
  for (int k = 0; k < item.size(); k++) {
    if (i == k || j == k) {
      continue;
    } else {
      add_operand(item[k]);
    }
  }
}

void User::remove_useof_operand() {
  int i = 0;
  for (auto op : operands_) {
    op->remove_use(this, i);
    i++;
  }
}