#include "Module.h"
#include "Type.h"
#include "BasicBlock.h"
#include <iterator>
#include <list>
#include <cstddef>
#include <map>
#include <cassert>

class Function: public Value{
public:
    Function (FunctionType* type, const std::string &name, Module *parent);
    ~Function();
    static Function *create_function(const std::string &id, Module *m, FunctionType *type, vector args_);
    FunctionType *get_function_type() const;
    Type *get_return_type() const;
    void add_basic_block(BasicBlock *bb);
    unsigned get_num_of_args() const;
    unsigned get_num_basic_blocks() const;
    Module *get_parents() const;
    std::list<arguments *>::iterator arg_begin();
    std::list<arguments *>::iterator arg_end();
    void remove(BasicBlock *bb);
    std::list<BasicBlock *> &get_basic_blocks();
    std::list<Argument *> &get_args();
    void set_instr_name();
private:
    FunctionType *type;
    Module *parent;
    std::vector<Value*> arguments;
    std::list<BasicBlock *> basic_block;
    std::string *id;
}