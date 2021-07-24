#include "IR/Module.h"
#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include "IR/IRprinter.h"
#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                      Function *parent = nullptr, bool fake=false)
    : Value(Type::get_label_type(m), name), parent_(parent), _fake(fake)
{
    assert(parent && "currently parent should not be nullptr");
    parent_->add_basic_block(this);
}

Module *BasicBlock::get_module()
{
    return get_parent()->get_parent();
}

void BasicBlock::add_instruction(Instruction *instr)
{
    instr->setSuccInst(nullptr);
    if (instr_list_.empty()) {
        instr->setSuccInst(nullptr);
    } else {
        Instruction *last_inst = instr_list_.back();
        instr->setPrevInst(last_inst);
        last_inst->setSuccInst(instr);
    }
    instr_list_.push_back(instr);
}

void BasicBlock::add_instr_begin(Instruction *instr)
{
    instr->setPrevInst(nullptr);
    if (instr_list_.empty()) {
        instr->setSuccInst(nullptr);
    } else {
        Instruction *first_inst = instr_list_.front();
        instr->setSuccInst(first_inst);
        first_inst->setPrevInst(instr);
    }
    instr_list_.push_front(instr);
}

void BasicBlock::add_instr_after_phi(Instruction *instr) {
    instr->set_parent(this);
    auto it = instr_list_.begin();
    for (; it != instr_list_.end(); ++it) {
        if (!(*it)->is_phi()) {
            break;
        }
    }
    Instruction *front = nullptr, *back = nullptr;
    if (it != instr_list_.begin())  {
        front = *(--it);
        ++it;
    }
    if (it != instr_list_.end()) {
        back = *it;
    }
    if (front != nullptr) {
        front->setSuccInst(instr);
    }
    if (back != nullptr) {
        back->setPrevInst(instr);
    }
    instr->setPrevInst(front);
    instr->setSuccInst(back);
    instr_list_.insert(it, instr);
}

void BasicBlock::delete_instr_simple(Instruction *instr) {
    instr_list_.remove(instr);
    Instruction *prev = instr->getPrevInst();
    Instruction *succ = instr->getSuccInst();
    if (prev != nullptr) {
        prev->setSuccInst(succ);
    }
    if (succ != nullptr) {
        succ->setPrevInst(prev);
    }
}

void BasicBlock::delete_instr( Instruction *instr )
{
    instr_list_.remove(instr);
    Instruction *prev = instr->getPrevInst();
    Instruction *succ = instr->getSuccInst();
    if (prev != nullptr) {
        prev->setSuccInst(succ);
    }
    if (succ != nullptr) {
        succ->setPrevInst(prev);
    }
    instr->remove_use_of_ops();
}

const Instruction *BasicBlock::get_terminator() const
{
    if (instr_list_.empty()){
        return nullptr;
    }
    switch (instr_list_.back()->get_instr_type())
    {
    case Instruction::ret:
        return instr_list_.back();
        break;
    
    case Instruction::br:
        return instr_list_.back();
        break;

    default:
        return nullptr;
        break;
    }
}

void BasicBlock::erase_from_parent()
{
    this->get_parent()->remove(this);
}

std::string BasicBlock::print()
{
    if (_fake) {
        return "";
    }
    std::string bb_ir;
    bb_ir += this->get_name();
    bb_ir += ":";
    // print prebb
    if(!this->get_pre_basic_blocks().empty())
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

    // 空BasicBlock，自动加上return语句
    if (get_terminator() == nullptr) {
        bb_ir += "  ";
        if(get_parent()->get_return_type()->is_void_type()) {
            bb_ir += "ret void\n";
        } else {
            bb_ir += "ret i32 0\n";
        }
    }

    return bb_ir;
}