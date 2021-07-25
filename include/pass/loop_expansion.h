
#ifndef SYSYC_LOOPEXP_H
#define SYSYC_LOOPEXP_H

#include "IR/Module.h"
#include "IR/Function.h"
#include "IR/IRbuilder.h"
#include "IR/BasicBlock.h"
#include "IR/Instruction.h"
#include "pass.h"
#include "stack"
#include "loop_search.h"

class Node;
class Phi_Node;
class Judge_Node;
class Loop_Node;

class LoopExpansion : public Pass{
private:
    std::map<Value *, Node *> ins2node;
    std::map<Value *, Loop_Node *> loop_nodes;
    std::map<Value *, int> loop_vars;
    std::map<Value *, Phi_Node *> target_phi;
    std::set<Value *> target;
    std::set<Value *> base;
public:
    LoopExpansion(Module *m) : Pass(m){}
    ~LoopExpansion(){};
    std::map<Value *, Node *> get_ins2node() { return ins2node; }
    void run() override;
    bool update_base_value(BasicBlock *loop_block);
};

class Node{
public:
    explicit Node(){};
    virtual ~Node() = default;
private:

};

class Loop_Node : public Node{
private:
    int node_val;
    Value *op1;
    Value *op2;
public:
    Loop_Node(Value *op1, Value *op2){
        this->op1 = op1;
        this->op2 = op2;
    }
    void set_node_val(int val) { this->node_val = val;}
    int get_node_val() { return node_val; }
};

class Judge_Node : public Node{
private:
    int node_val;
    BinaryInst::OpID op;
    Value *instr;
    Value* op1 = nullptr;
    Value* op2 = nullptr;
public:
    Judge_Node(Value *instr, Value *op1, Value *op2, BinaryInst::OpID op) {
        this->instr = instr;
        this->op1 = op1;
        this->op2 = op2;
        this->op = op;
    }
    void set_node_val(int val) { this->node_val = val;}
    int get_node_val() { return node_val; }
    int calculate_judge_value(std::map<Value *, Node *> ins2node);
};

class Phi_Node : public Node{
private:
    int init_val;
    Value *update_value;
public:
    Phi_Node(int init_val, Value *update_value) {
        this->init_val = init_val;
        this->update_value = update_value;
    }
    int get_val() { return init_val; }
    void set_val(int val_) { this->init_val = val_; }
};

#endif