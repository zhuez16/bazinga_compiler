//
// Created by 顾超 on 2021/7/30.
//

#include <list>
#include <cassert>
#include <string>
#include <vector>
#include <map>

#include "IR/Function.h"


class ASValue;
class ASBlock;
class ASFunction;

struct ASUse {
    ASValue * _user;
    unsigned _arg_id;
    ASUse(ASValue *usr, unsigned idx) : _user(usr), _arg_id(idx) {}
    bool operator== (const ASUse &rhs) const {
        return _user == rhs._user && _arg_id == rhs._arg_id;
    }
};

class ASValue {
public:
    enum AS_TY {
        ASConstantTy,
        ASBlockTy,
        ASInstructionTy,
        ASGlobalTy
    };
private:
    AS_TY _ty;
    std::vector<ASValue *> _operands;
    std::list<ASUse> _use;
    std::string _name;
protected:
    explicit ASValue(unsigned num_operands) { _operands.resize(num_operands); }
public:
    std::list<ASUse> getUseList() const { return _use; }
    ASValue *getOperand(unsigned idx) const { return _operands[idx]; }
    std::vector<ASValue *> getOperands() { return _operands; }
    std::vector<ASValue *> getOperandsWithOp2();
    void setOperand(unsigned idx, ASValue *v);
    void addUser(unsigned idx, ASValue *user) { _use.emplace_back(user, idx); }
    void removeUser(unsigned idx, ASValue *user) {
        ASUse tbd(user, idx);
        for (auto it = _use.begin(); it != _use.end(); ++it) {
            if (*it == tbd) {
                _use.erase(it);
                break;
            }
        }
    }
    virtual std::string print() { return ""; }
    std::string getName() const { return _name; }
    void setName(std::string n) { _name = n; }
    void expandNumOperand(int by) { _operands.resize(_operands.size() + by); }
};

class ASArgument : public ASValue {
private:

public:
    static ASArgument *createArgument();
};

class ASGlobalValue : public ASValue {
private:
    bool _array;
    int _size;
    std::vector<int> _initial;
public:
    bool isArray() const { return _array; }
    int getArraySize() const { assert(isArray()); return _size; }
    int getInitialValue() const { return _initial[0]; }
    std::vector<int> getArrayInitial() { return _initial; }
};

class ASConstant : public ASValue {
    // We don't maintain the use list of ASConstant type
private:
    int _value;
    explicit ASConstant(int val) : ASValue(0),  _value(val) {}
public:
    static ASConstant *getConstant(int constVal) { return new ASConstant(constVal); }
    int getValue() const { return _value; }
};

class ASInstruction : public ASValue {
    // This is a base class of all instructions
public:
    enum ASMInstType {
        ASMAddTy,   // [x] Rd = Rn + Op2
        ASMSubTy,   // [x] Rd = Rn - Op2
        ASMMulTy,   // [x] Rd = Rm * Rs
        ASMMlaTy,   // Rd = Rn + Rm * Rs
        ASMMlsTy,   // Rd = Rn - Rm - Rs
        ASMDivTy,   // [x] Rd = Rn / Rm
        ASMMovTy,   // [x] Rd = Op2
        ASMMvnTy,   // [x] Rd = 0xFFFFFFFF ^ Op2
        ASMAsrTy,   // [x] Rd = Rm >>> Rs|Imm
        ASMLslTy,   // [x] Rd = Rm << Rs|Imm
        ASMLsrTy,   // [x] Rd = Rm >> Rs|Imm
        ASMCmpTy,   // [x] Rn - Op2 => CPSR
        ASMCmzTy,   // [x] Rn + Op2 => CPSR
        ASMBrTy,    // [x] PC <- Label / Lr = PC, PC <- Label, can apply condition
        ASMCbzTy,   // if Rn == 0, PC <- Label
        ASMCbnzTy,  // if Rn != 0, PC <- Label
        ASMPushTy,  // [x] Stack <- { R1, R2, ... }
        ASMPopTy,   // [x] { R1, R2, ... } <- Stack
        ASMLoadTy,  // [x] Rd = Mem[Rn (+ offset)]
        ASMStoreTy, // [x] Mem[Rn (+ offset)] = Rd
        ASMPhiTy,   // [x] Fake instruction
    };
protected:
    ASInstruction(unsigned num_op, ASMInstType ty) : ASValue(num_op), _ty(ty) {}
private:
    ASBlock *_parent{};
    ASMInstType _ty;
public:
    void setParent(ASBlock *b) { _parent = b; }
    ASBlock *getBlock() const { return _parent; }
    ASMInstType getInstType() const { return _ty; }
    // 若指令包含一个返回值 Rd，则为true，否则为false
    bool hasResult() const {
        auto ty = getInstType();
        return ty == ASMAddTy || ty == ASMSubTy || ty == ASMMulTy || ty == ASMDivTy || ty == ASMLsrTy ||
        ty == ASMLslTy || ty == ASMLoadTy || ty == ASMMovTy || ty == ASMMvnTy || ty == ASMAsrTy;
    }

};

class ASLabel : public ASValue {
    // Values that can be used as a label
protected:
    explicit ASLabel(std::string name) : ASValue(0) {
        setName(std::move(name));
    }
};

class ASBlock : public ASLabel {
private:
    std::list<ASInstruction *> _inst_list;
    ASFunction *_parent = nullptr;

    explicit ASBlock(std::string name) : ASLabel(std::move(name)) {}
public:
    void addInstruction(ASInstruction *inst) {
        inst->setParent(this);
        _inst_list.push_back(inst);
    }

    void setParent(ASFunction *f) { _parent = f; }
    ASFunction *getFunction() const { return _parent; }

    static ASBlock *createBlock(ASFunction *parent, std::string name) {
        auto ret = new ASBlock(std::move(name));
        ret->setParent(parent);
        return ret;
    }
};

class ASFunction : public ASLabel {
private:
    std::list<ASBlock *> _block_list;
    std::map<void *, int> _stack;
    int _sp_pointer = 0;

    explicit ASFunction(std::string name) : ASLabel(std::move(name)) {}
public:
    std::list<ASBlock *> getBlockList() { return _block_list; }
    void addBlock(ASBlock *block) { block->setParent(this); _block_list.push_back(block); }
    void eraseBlock(ASBlock *block) { block->setParent(nullptr); _block_list.remove(block); }

    // 在函数栈空间中分配存储
    int allocStack(void* base, int requiredSize) {
        int current_sp = _sp_pointer;
        _sp_pointer += requiredSize;
        _stack[base] = current_sp;
        return current_sp;
    }

    // 获取某个栈中的量相对与 sp 指针的偏移量
    int getStackOffset(void *base) {
        if (_stack.find(base) == _stack.end()) return -1;
        return _stack[base];
    }

    // 获取偏移量
    int getStackSize() const { return _sp_pointer; }

    static ASFunction *createFunction(std::string name) { return new ASFunction(std::move(name)); }
};

/**
 * ARM Operand 2 灵活操作数
 * 注意：灵活操作数的 Use / User 变更方式与平时不一样
 * 若将灵活操作数作为Operand传入Instruction中，则会将Instruction加入到Op2中所有Reg型变量的UseList中
 */
class ASOperand2 : public ASValue {
public:
    enum Op2Type {
        Op2ImmTy,
        Op2RegTy,
        Op2RegImm,
        Op2LslTy,
        Op2LsrTy,
        Op2AsrTy,
        Op2RorTy,
    };
private:
    Op2Type _ty;
    explicit ASOperand2(Op2Type ty) : ASValue(0), _ty(ty) {}
    ASValue *base = nullptr;
    ASValue *shift = nullptr;
public:
    static ASOperand2 *getOperand2(int c) {
        auto ret = new ASOperand2(Op2ImmTy);
        ret->base = ASConstant::getConstant(c);
        return ret;
    }

    static ASOperand2 *getOperand2(ASInstruction *i) {
        auto ret = new ASOperand2(Op2RegTy);
        ret->base = i;
        return ret;
    }

    // TODO: 其他 Op2 还没用到，先放着

    Op2Type getOp2Type() const { return _ty; }
    ASValue *getRm() const { return base; }
    ASValue *getRs() const { return shift; }
};

class ASBinaryInst : public ASInstruction {
private:
    ASBinaryInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASBinaryInst *createASMAdd(ASInstruction *Rn, ASOperand2 *Op2) {
        auto ret = new ASBinaryInst(ASMAddTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASBinaryInst *createASMSub(ASInstruction *Rn, ASOperand2 *Op2) {
        auto ret = new ASBinaryInst(ASMSubTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASBinaryInst *createASMMul(ASInstruction *Rm, ASInstruction *Rs) {
        auto ret = new ASBinaryInst(ASMMulTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMDiv(ASInstruction *Rn, ASInstruction *Rm) {
        auto ret = new ASBinaryInst(ASMDivTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Rm);
        return ret;
    }

    static ASBinaryInst *createASMAsr(ASInstruction *Rm, ASOperand2 *Rs) {
        auto ret = new ASBinaryInst(ASMAsrTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMLsl(ASInstruction *Rm, ASInstruction *Rs) {
        auto ret = new ASBinaryInst(ASMLslTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMLsr(ASInstruction *Rm, ASInstruction *Rs) {
        auto ret = new ASBinaryInst(ASMLsrTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }
};

class ASUnaryInst : public ASInstruction {
private:
    ASUnaryInst(ASMInstType ty) : ASInstruction(1, ty) {}

public:
    static ASUnaryInst *createASMMov(ASOperand2 *Op2) {
        auto ret = new ASUnaryInst(ASMMovTy);
        ret->setOperand(0, Op2);
        return ret;
    }

    static ASUnaryInst *createASMMvn(ASOperand2 *Op2) {
        auto ret = new ASUnaryInst(ASMMvnTy);
        ret->setOperand(0, Op2);
        return ret;
    }
};

class ASCmpInst : public ASInstruction {
private:
    ASCmpInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASCmpInst *createASMCmp(ASInstruction *Rn, ASOperand2 *Op2) {
        auto ret = new ASCmpInst(ASMCmpTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASCmpInst *createASMCnz(ASInstruction *Rn, ASOperand2 *Op2) {
        auto ret = new ASCmpInst(ASMCmzTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }
};

class ASBranchInst : public ASInstruction {
public:
    enum ASMBranchCond {
        CondNo,
        CondEQ,
        CondNE,
        CondLT,
        CondLE,
        CondGT,
        CondGE
    };
private:
    ASMBranchCond _cond;
    bool _withLink;
    ASBranchInst(ASMBranchCond cond, bool link, ASLabel *label) : ASInstruction(1, ASMBrTy), _cond(cond), _withLink(link) {
        setOperand(0, label);
    }

public:
    static ASBranchInst *createBranch(ASLabel *lbl) { return new ASBranchInst(CondNo, false, lbl); }
    static ASBranchInst *createCondBranch(ASLabel *lbl, ASMBranchCond c) { return new ASBranchInst(c, false, lbl); }
    static ASBranchInst *createLinkBranch(ASLabel *lbl) { return new ASBranchInst(CondNo, true, lbl); }
    static ASBranchInst *createReturnBranch() { return new ASBranchInst(CondNo, true, nullptr); } // BX
    std::string print() final;
    ASLabel *getLabel() const { return dynamic_cast<ASLabel *>(getOperand(0)); }
};

class ASPushInst : public ASInstruction {
private:
    ASPushInst() : ASInstruction(0, ASMPushTy) {}

public:
    ASPushInst *createPush() { return new ASPushInst(); }
};

class ASPopInst : public ASInstruction {
private:
    ASPopInst() : ASInstruction(0, ASMPopTy) {}

public:
    ASPopInst *createPush() { return new ASPopInst(); }
};

class ASLoadInst : public ASInstruction {
private:
    ASLoadInst(ASInstruction *Rn) : ASInstruction(1, ASMLoadTy) {
        setOperand(0, Rn);
    }
    ASLoadInst(ASInstruction *Rn, ASOperand2 *Op2) : ASInstruction(2, ASMLoadTy) {
        setOperand(0, Rn);
        setOperand(1, Op2);
    }
};

class ASStoreInst : public ASInstruction {
private:
    ASStoreInst(ASInstruction *Rd, ASInstruction *Rn) : ASInstruction(2, ASMLoadTy) {
        setOperand(0, Rd);
        setOperand(1, Rn);
    }
    ASStoreInst(ASInstruction *Rd, ASInstruction *Rn, ASOperand2 *Op2) : ASInstruction(3, ASMLoadTy) {
        setOperand(0, Rd);
        setOperand(1, Rn);
        setOperand(2, Op2);
    }
};

class ASPhiInst : public ASInstruction {
private:
    ASPhiInst() : ASInstruction(0, ASMPhiTy) {}

public:
    void addPhiPair(ASBlock *from, ASValue *value) {
        expandNumOperand(2);
        setOperand(0, from);
        setOperand(1, value);
    }
};

/**
 * 固定的寄存器。这些寄存器无需进行寄存器分配且必须满足要求
 */
class ASFixedRegister : public ASValue {
private:
    explicit ASFixedRegister(int id) : ASValue(0), regId(id) {}

    int regId;
public:
    int getRegID() const { return regId; }
    static ASFixedRegister *getRegister(int regId) { return new ASFixedRegister(regId); }
};