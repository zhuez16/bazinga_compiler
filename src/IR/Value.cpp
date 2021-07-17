#include "Value.h"
//#include "ReturnVal.h"
#include "Type.h"
#include "User.h"
#include <algorithm>
#include <cassert>

Value::Value(Type *ty, const std::string &name) : type(ty), name_(name) {}

void Value::add_use(Value *val, unsigned arg_no) {
  use_list_.push_back(Use(val, arg_no));
}

std::string Value::get_name() const { return name_; }

void Value::replace_use_list(Value *new_val) {
  for (auto use : use_list_) {
    auto val = dynamic_cast<User *>(use.val_);
    //exit_ifnot(_EmptyUse_replaceAllUseWith_Value, val);
    val->set_operand(use.arg_no_, new_val);
  }
}

void Value::remove_use(Value *val, unsigned arg_no) {
  auto temp = std::find(use_list_.begin(), use_list_.end(), Use(val, arg_no));
  //exit_ifnot(_CantFindUse_removeUse_Value, temp != use_list_.end());
  use_list_.erase(temp);
}