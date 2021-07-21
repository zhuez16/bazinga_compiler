#include "IR/Value.h"
#include "IR/Type.h"
#include "IR/User.h"
#include "IR/BasicBlock.h"
#include <cassert>

Value::Value(Type *ty, const std::string &name )
  : type_(ty), name_(name)
{

}

void Value::add_use(Value *val, unsigned arg_no )
{
    use_list_.push_back(Use(val, arg_no));
}

std::string Value::get_name() const
{
    return name_;
}

void Value::replace_all_use_with(Value *new_val)
{
    for (auto use : use_list_) {
        auto bb = dynamic_cast<BasicBlock *>(use.val_);
        if (bb) {
            auto bbori =dynamic_cast<BasicBlock *>(this);
            bb->replace_basic_block(bbori, bb);
            return;
        }

        auto val = dynamic_cast<User *>(use.val_);
        assert(val && "new_val is not a user");
        val->set_operand(use.arg_no_, new_val);
    }
}

void Value::remove_use(Value *val)
{
    auto is_val = [val] (const Use &use) { return use.val_ == val; };
    use_list_.remove_if(is_val);
}