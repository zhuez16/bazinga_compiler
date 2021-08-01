#ifndef BAZINGA_POWER_SUM_H
#define BAZINGA_POWER_SUM_H

#include "pass_manager.h"

class CR;
class power_sum_delete;
/**
 * CR is a structure like (init, +(*), step)
 * for example:
 * @code
 * int i = 0;
 * int res = 0;
 * while(i < n){
 *      i = i + 1;
 *      res = res + i;
 * }
 * @endcode
 * i can be describe as (0, +, 1)
 * res can be describe as (0, +, i) = (0, +, (1, +, 1))
 * thus i and res in k iteration can be calculate
 * i: 0 + 1 * k = k
 * res: 0 + 1 * k + 1 * k * (k - 1) / 2 = k * (k + 1) / 2
 */

class CR{
public:
    CR(){
        this->init_val = nullptr;
        this->step = nullptr;
    }
    CR(Value *init_val){
        this->init_val = init_val;
        this->step = nullptr;
    }
    CR(Value *init_val, CR *step){
        this->init_val = init_val;
        this->step = step;
    }
    void set_init(Value *init_val){
        this->init_val = init_val;
    }
    void set_step(CR *step){
        this->step = step;
    }
    Value* get_init_val(){
        return this->init_val;
    }
    CR* get_step(){
        return this->step;
    }
    Value *create_i_item(int i, Value *loop_time, BasicBlock *bb, Module *m, Value *init_val){
        int div_item = factor_(i);
        if(i == 0){
            return init_val;
        }
        else if(i == 1){
            return BinaryInst::create_mul(init_val, loop_time, bb, m);
        }
        else{
            int k = 1;
            Value *temp = BinaryInst::create_mul(init_val, loop_time, bb, m);
            while(k <= i - 1){
                auto sub_instr = BinaryInst::create_sub(loop_time, ConstantInt::get(k, m), bb, m);
                auto mul_instr = BinaryInst::create_mul(temp, sub_instr, bb, m);
                temp = mul_instr;
                k++;
            }
            auto div_instr = BinaryInst::create_sdiv(temp, ConstantInt::get(div_item, m), bb, m);
            return div_instr;
        }
    }
    Value *gen_code_for_cr(Value *loop_time, BasicBlock *bb, Module *m){
        CR *ptr = this;
        std::vector<Value *> items;
        items.push_back(create_i_item(0, loop_time, bb, m, ptr->get_init_val()));
        int i = 1;
        while(ptr->get_step() != nullptr){
            ptr = ptr->get_step();
            items.push_back(create_i_item(i, loop_time, bb, m, ptr->get_init_val()));
            i ++;
        }
        if(items.size() == 1){
            return items[0];
        }
        else{
            auto temp = items[0];
            for(int k = 1; k < items.size(); k++){
                auto add_instr = BinaryInst::create_add(temp, items[k], bb, m);
                temp = add_instr;
            }
            return temp;
        }
    }
    /**
     * add rule
     * {x1, +, x2} + {y1, +, y2} = {x1 + y1, +, x2 + y2}
     */
    CR *add(CR *cr1, CR *cr2, Module *m, BasicBlock *bb) {
        auto new_cr = new CR();
        auto init_val_1 = cr1->get_init_val();
        auto init_val_2 = cr2->get_init_val();
        auto const_init_val_1 = dynamic_cast<ConstantInt *>(init_val_1);
        auto const_init_val_2 = dynamic_cast<ConstantInt *>(init_val_2);
        Value *init_val_;
        if(const_init_val_1 && const_init_val_2){
            int val = const_init_val_1->get_value() + const_init_val_2->get_value();
            init_val_ = ConstantInt::get(val, m);
        }
        else{
            init_val_ = BinaryInst::create_add(init_val_1, init_val_2, bb, m);
        }
        new_cr->set_init(init_val_);
        if(cr1->get_step() == nullptr && cr2->get_step() == nullptr){
            ;
        }
        else if(cr1->get_step() == nullptr){
            new_cr->set_step(cr2->get_step());
        }
        else if(cr2->get_step() == nullptr){
            new_cr->set_step(cr1->get_step());
        }
        else{
            new_cr->set_step(add(cr1->get_step(), cr2->get_step(), m, bb));
        }
        return new_cr;
    }
    CR *sub(CR *cr1, CR *cr2, Module *m, BasicBlock *bb) {
        auto new_cr = new CR();
        auto init_val_1 = cr1->get_init_val();
        auto init_val_2 = cr2->get_init_val();
        auto const_init_val_1 = dynamic_cast<ConstantInt *>(init_val_1);
        auto const_init_val_2 = dynamic_cast<ConstantInt *>(init_val_2);
        Value *init_val_;
        if(const_init_val_1 && const_init_val_2){
            int val = const_init_val_1->get_value() - const_init_val_2->get_value();
            init_val_ = ConstantInt::get(val, m);
        }
        else{
            init_val_ = BinaryInst::create_sub(init_val_1, init_val_2, bb, m);
        }
        new_cr->set_init(init_val_);
        if(cr1->get_step() == nullptr && cr2->get_step() == nullptr){
            ;
        }
        else if(cr1->get_step() == nullptr){
            new_cr->set_step(cr2->get_step());
        }
        else if(cr2->get_step() == nullptr){
            new_cr->set_step(cr1->get_step());
        }
        else{
            new_cr->set_step(sub(cr1->get_step(), cr2->get_step(), m, bb));
        }
        return new_cr;
    }
    /**
     * mul const rule
     * c * {x1, +, x2} = {c * x1, +, c * x2}
     */
    CR *mul_const(CR *cr1, Value *const_val, Module *m, BasicBlock *bb){
        auto new_cr = new CR();
        auto init_val_ = cr1->get_init_val();
        auto step_ = cr1->get_step();
        auto const_init_val_ = dynamic_cast<ConstantInt *>(init_val_);
        auto const_mul = dynamic_cast<ConstantInt *>(const_val);
        Value *new_init_val;
        if(const_init_val_ && const_mul){
            int mul_val = const_init_val_->get_value() * const_mul->get_value();
            new_init_val = ConstantInt::get(mul_val, m);
        }
        else{
            new_init_val = BinaryInst::create_mul(init_val_, const_val, bb, m);
        }
        new_cr->set_init(new_init_val);
        if(step_ != nullptr){
            new_cr->set_step(mul_const(step_, const_val, m, bb));
        }
        return new_cr;
    }
    /**
     * mul rule
     * {x1, +, x2} * {y1, +, y2} = {x1 * y1, x1 * {y1, +, y2} + y1 * {x1, +, x2} + x2 * y2}
     */
    CR *mul(CR *cr1, CR *cr2, Module *m, BasicBlock *bb){
        auto new_cr = new CR();
        auto init_val_1 = cr1->get_init_val();
        auto init_val_2 = cr2->get_init_val();
        auto const_init_val_1 = dynamic_cast<ConstantInt *>(init_val_1);
        auto const_init_val_2 = dynamic_cast<ConstantInt *>(init_val_2);
        Value *new_init_val;
        if(const_init_val_1 && const_init_val_2){
            int val = const_init_val_1->get_value() + const_init_val_2->get_value();
            new_init_val = ConstantInt::get(val, m);
        }
        else{
            new_init_val = BinaryInst::create_mul(init_val_1, init_val_2, bb, m);
        }
        new_cr->set_init(new_init_val);
        auto step_val_1 = cr1->get_step();
        auto step_val_2 = cr2->get_step();
        if(step_val_1->step == nullptr && step_val_2->step == nullptr){
//            std::cout << "debug1" << std::endl;
            auto val_1 = step_val_1->get_init_val();
//            std::cout << "debug2" << std::endl;
            auto val_2 = step_val_2->get_init_val();
//            std::cout << "debug3" << std::endl;
            auto step_cr_1 = mul_const(cr2, val_1, m, bb);
            auto step_cr_2 = mul_const(cr1, val_2, m, bb);
//            std::cout << "debug4" << std::endl;
            auto step_cr_3 = BinaryInst::create_mul(val_1, val_2, bb, m);
            auto temp = add(step_cr_1, step_cr_2, m, bb);
            auto temp_cr = new CR();
//            std::cout << "debug5" << std::endl;
            temp_cr->set_init(step_cr_3);
            new_cr->set_step(add(temp, temp_cr, m, bb));
//            std::cout << "debug6" << std::endl;
        }
        else if(step_val_1->step == nullptr){
            auto val_1 = step_val_1->get_init_val();
            auto step_cr_1 = mul_const(cr2, val_1, m, bb);
            auto step_cr_2 = mul(cr1, step_val_2, m, bb);
            auto step_cr_3 = mul_const(step_val_2, val_1, m, bb);
            auto temp = add(step_cr_1, step_cr_2, m, bb);
            new_cr->set_step(add(temp, step_cr_3, m, bb));
        }
        else if(step_val_2->get_step() == nullptr){
            auto val_2 = step_val_2->get_init_val();
            auto step_cr_1 = mul_const(cr1, val_2, m, bb);
            auto step_cr_2 = mul(cr2, step_val_1, m, bb);
            auto step_cr_3 = mul_const(step_val_1, val_2, m, bb);
            auto temp = add(step_cr_1, step_cr_2, m, bb);
            new_cr->set_step(add(temp, step_cr_3, m, bb));
        }
        else{
            auto step_cr_1 = mul(step_val_1, cr2, m, bb);
            auto step_cr_2 = mul(step_val_2, cr1, m, bb);
            auto step_cr_3 = mul(step_val_1, step_val_2, m, bb);
            auto temp = add(step_cr_1, step_cr_2, m, bb);
            new_cr->set_step(add(temp, step_cr_3, m, bb));
        }
        return new_cr;
    }
private:
    int factor_(int n){
        if(n == 0 || n == 1){
            return 1;
        }
        else{
            return n * factor_(n - 1);
        }
    }
    Value *init_val;
    CR *step = nullptr;
};

class power_sum_delete : public Pass {
public:
    explicit power_sum_delete(Module *m) : Pass(m) {}
    void run() final;
    BasicBlock *bb;
private:
};

class trace_node;

class trace_node{
public:
    trace_node(Value *use){
        this->use = use;
        this->root = nullptr;
    }
    void set_trace(trace_node *node){
        this->root = node;
    }
    trace_node *root;
    Value *use;
};

#endif