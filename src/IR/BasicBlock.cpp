#include "Module.h"
#include "BasicBlock.h"
#include "Function.h"
#include "IRprinter.h"
#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                      Function *parent = nullptr)
    : Value(Type::get_Label_type(m), name), parent(parent)
{
    assert(parent && "currently parent should not be nullptr");
    parent->add_basic_block(this);
}


Module *BasicBlock::get_module(){
    return get_parents()->get_parents();
}
void BasicBlock::push_back_instr(Instruction *instr){
    instr_list.push_back(instr);
}
void BasicBlock::push_front_instr(Instruction *instr){
    instr_list.push_front(instr);
}
void BasicBlock::delete_instruction(Instruction *instr){
    instr_list.remove(instr);
}
bool BasicBlock::is_empty(){
    return !instr_list.empty();
}
int BasicBlock::get_num_of_instr(){
    return instr_list.size();
}
std::list<Instruction *> &BasicBlock::get_instructions(){
    return instr_list;
}
void erase_from_parents(){
    this->get_parent()->remove(this);
}

std::string BasicBlock::print()
{
    std::string bb_ir;
    bb_ir += this->get_name();
    bb_ir += ":";
    // print prebb
    if(!this->get_pre_basic_block().empty())
    {
        bb_ir += "                                                ; preds = ";
    }
    for (auto bb : this->get_pre_basic_blocks() )
    {
        if( bb != *this->get_pre_basic_blocks().begin() )
            bb_ir += ", ";
        bb_ir += print_as_op(bb, false);
    }
    
    // print prebb
    if ( !this->get_parent() )
    {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
    for ( auto instr : this->get_instructions() )
    {
        bb_ir += "  ";
        bb_ir += instr->print();
        bb_ir += "\n";
    }

    return bb_ir;
}
