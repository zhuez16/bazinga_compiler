#ifndef SYSYC_VALUE_H
#define SYSYC_VALUE_H

#include <iostream>
#include <list>
#include <string>
// #include "Constant.h"

class Type;
class Value;

struct Use {
  Value *val_;
  unsigned arg_no_; // the no. of operand, e.g., func(a, b), a is 0, b is 1
  Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}
  friend bool operator==(const Use &lhs, const Use &rhs) {
    return lhs.val_ == rhs.val_ && lhs.arg_no_ == rhs.arg_no_;
  }
};

class UseHash {
public:
  size_t operator()(const Use &u) const {
    return (std::hash<Value *>()(u.val_)) ^ (std::hash<unsigned>()(u.arg_no_));
  }
};

class Value {
public:
  explicit Value(Type *ty, const std::string &name = "");
  ~Value() = default;

  Type *get_Type() const { return type; }

  std::list<Use> &get_use_list() { return use_list_; }

  void add_use(Value *val, unsigned arg_no = 0);

  bool set_name(std::string name) {
    if (name_ == "") {
      name_ = name;
      return true;
    }
    return false;
  }
  std::string get_name() const;

  virtual void print() {}

  // replace all uses of this value to new_val
  // e.g., replace a = this_value + b to a = new_value + b
  void replace_use_list(Value *new_val);

  void remove_use(Value *val, unsigned arg_no);

  // bool isValidVar() {(type_->isIntegerTy() &&
  // dynamic_cast<ConstantInt*>(this)) || type_->isPointerTy();}
private:
  Type *type;
  std::list<Use> use_list_; // who use this value
  std::string name_;        // should we put name field here ?
};

#endif // SYSYC_VALUE_H
