#ifndef SYSYC_BASICBLOCK_H
#define SYSYC_BASICBLOCK_H

#include "Value.h"
#include "Instruction.h"
#include "Module.h"
#include "Function.h"

#include <list>
#include <set>
#include <string>

class Function;

class Instruction;

class Module;

class BasicBlock : public Value {
private:
    explicit BasicBlock(Module *m, const std::string &name, Function *parent);
    std::string name_;
    std::list<BasicBlock *> prev_bbs;
    std::list<BasicBlock *> succ_bbs;
    std::list<Instruction *> instr_list;
    Function *parent;

public:
    static BasicBlock *create(Module *m, const std::string &name, Function *parent) {
        auto prefix = name.empty() ? "" : "label_";
        return new BasicBlock(m, prefix + name, parent);
    }

    std::string get_name() {
        return name_;
    }

    Function *get_parents() {
        return parent;
    }

    std::list<BasicBlock *> &get_prev_bbs() { return prev_bbs; }

    std::list<BasicBlock *> &get_succ_bbs() { return succ_bbs; }

    void add_prev_bb(BasicBlock *bb) { prev_bbs.push_back(bb); }

    void add_succ_bb(BasicBlock *bb) { succ_bbs.push_back(bb); }

    void remove_prev_bb(BasicBlock *bb) { prev_bbs.remove(bb); }

    void remove_succ_bb(BasicBlock *bb) { succ_bbs.remove(bb); }


    const Instruction *get_terminator() const;

    Instruction *get_terminator() {
        return const_cast<Instruction *>(
                static_cast<const BasicBlock *>(this)->get_terminator());
    }

    Module *get_module();

    void push_back_instr(Instruction *instr);

    void push_front_instr(Instruction *instr);

    void delete_instruction(Instruction *instr);

    bool is_empty();

    int get_num_of_instr();

    std::list<Instruction *> &get_instructions();

    void erase_from_parents();

    virtual std::string print() override;


};

#endif 
