#include "Module.h"
#include "Function.h"
#include "IRprinter.h"

Function::Function (FunctionType* type, const std::string &name, Module *parent,vector args_){
    parent->add_function(this);
    arguments=args;
}

static Function *Function::create_function(std::string &id, Module *m, Type *type, vector args_){
    return new Function(type,id,m,args_);
}
FunctionType *Function::get_function_type() const
{
    return static_cast<FunctionType *>(get_type());
}

Type *Function::get_return_type() const
{
    return get_function_type()->get_return_type();
}
void Function::add_basic_block(BasicBlock *bb){
    basic_block.push_back(bb);
}
unsigned Function::get_num_of_args(){
    return arguments.size();
}
unsigned Function::get_num_basic_blocks(){
    return basic_block.size();
}
Module *Function::get_parents(){
    return parent;
}
std::list<Argument *>::iterator Function::arg_begin(){
    return arguments.begin();
}
std::list<Argument *>::iterator Function::arg_end(){
    return arguments.end();
}
void Function::remove(BasicBlock *bb){
    basic_block.remove(bb);
}
std::list<BasicBlock *> &Function::get_basic_blocks(){
    return *basic_block;
}
std::list<Argument *> &Function::get_args(){
    return *arguments;
}
void Function::set_instr_name()
{
    std::map<Value *, int> seq;
    for (auto arg : this->get_args())
    {
        if ( seq.find(arg) == seq.end())
        {
            auto seq_num = seq.size() + seq_cnt_;
            if ( arg->set_name("arg"+std::to_string(seq_num) ))
            {
                seq.insert( {arg, seq_num} );
            }
        }
    }
    for (auto bb : basic_blocks_)
    {
        if ( seq.find(bb) == seq.end())
        {
            auto seq_num = seq.size() + seq_cnt_;
            if ( bb->set_name("label"+std::to_string(seq_num) ))
            {
                seq.insert( {bb, seq_num} );
            }
        }
        for (auto instr : bb->get_instructions())
        {
            if ( !instr->is_void() && seq.find(instr) == seq.end())
            {
                auto seq_num = seq.size() + seq_cnt_;
                if ( instr->set_name("op"+std::to_string(seq_num) ))
                {
                    seq.insert( {instr, seq_num} );
                }
            }
        }
    }
    seq_cnt_ += seq.size();
}
