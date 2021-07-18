#ifndef SYSYC_FUNCTION_H
#define SYSYC_FUNCTION_H

#include "Module.h"
#include "Type.h"
#include "BasicBlock.h"
#include <iterator>
#include <list>
#include <cstddef>
#include <map>
#include <cassert>
#include <vector>

class Function: public Value{
public:
    Function (FunctionType* type, const std::string &name, Module *parent);
    ~Function();
    static Function *create_function(const std::string &id, Module *m, FunctionType *type);
    FunctionType *get_function_type() const;
    Type *get_return_type() const;
    void add_basic_block(BasicBlock *bb);
    unsigned get_num_of_args() const;
    unsigned get_num_basic_blocks() const;
    Module *get_parents() const;
    std::vector<Value *>::iterator arg_begin();
    std::vector<Value *>::iterator arg_end();
    void remove(BasicBlock *bb);
    std::list<BasicBlock *> &get_basic_blocks();
    std::vector<Value *> &get_args();
    void set_instr_name();
private:
    FunctionType *type;
    Module *parent;
    std::vector<Value*> arguments;
    std::list<BasicBlock *> basic_block;
    std::string *id;
};

#endif