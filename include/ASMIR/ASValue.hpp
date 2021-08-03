//
// Created by 顾超 on 2021/7/30.
//

#include <list>
#include <cassert>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>

#include "IR/Function.h"


class ASValue;

class ASBlock;

class ASFunction;

class RegMapper;

struct ASUse {
    ASValue *_user;
    unsigned _arg_id;

    ASUse(ASValue *usr, unsigned idx) : _user(usr), _arg_id(idx) {}

    bool operator==(const ASUse &rhs) const {
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
    AS_TY _ty {};
    std::vector<ASValue *> _operands;
    std::list<ASUse> _use;
    std::string _name;
protected:
    explicit ASValue(unsigned num_operands) { _operands.resize(num_operands); }

public:
    std::list<ASUse> getUseList() const ;
    ASValue *getOperand(unsigned idx) const ;

    std::vector<ASValue *> getOperands() const ;

    int getNumOperands() const ;

    std::vector<ASValue *> getOperandsWithOp2();

    void setOperand(unsigned idx, ASValue *v);

    void addUser(unsigned idx, ASValue *user) ;

    void removeUser(unsigned idx, ASValue *user) ;
    virtual std::string print(RegMapper *mapper) ;

    std::string getName() const ;

    void setName(std::string n) ;

    void expandNumOperand(unsigned by) ;

    std::string printUser(RegMapper *mapper);
};

class ASArgument : public ASValue {
private:
    ASArgument() : ASValue(0) {}
public:
    static ASArgument *createArgument() ;

    std::string print(RegMapper *mapper) final;

};

class ASGlobalValue : public ASValue {
private:
    bool _array{};
    int _size{};
    std::vector<int> _initial;
    ASGlobalValue(std::string name, std::vector<int> init) : ASValue(0), _initial(std::move(init)) {
        setName(std::move(name));
    }

public:
    bool isArray() const { return _array; }

    int getArraySize() const ;

    int getInitialValue() const ;

    std::vector<int> getArrayInitial();
    static ASGlobalValue *create(std::string name, Type *ty, Constant *init = nullptr) ;

    std::string print(RegMapper *mapper) final;
};

class ASConstant : public ASValue {
    // We don't maintain the use list of ASConstant type
private:
    int _value;

    explicit ASConstant(int val) : ASValue(0), _value(val) {}

public:
    static ASConstant *getConstant(int constVal) ;

    int getValue() const ;

    std::string print(RegMapper *mapper) final;
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
        ASMCallTy,  // [x] Fake instruction
        ASMRetTy,   // [x] Fake instruction
    };

    ASInstruction(unsigned num_op, ASMInstType ty) : ASValue(num_op), _ty(ty) {}

protected:

private:
    ASBlock *_parent{};
    ASMInstType _ty;
public:
    void setParent(ASBlock *b) ;

    ASBlock *getBlock() const ;

    ASMInstType getInstType() const ;

    // 若指令包含一个返回值 Rd，则为true，否则为false
    bool hasResult() const;
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
    std::list<std::string> _inst_print;
    ASFunction *_parent = nullptr;

    explicit ASBlock(std::string name) : ASLabel(std::move(name)) {}

public:
    std::list<std::string> get_asm_inst();
    void addInstruction(ASInstruction *inst) ;
    void addInstruction(std::string inst);

    std::list<ASInstruction *> getInstList();

    std::list<ASInstruction *> getReverseInstList();

    void setParent(ASFunction *f) ;

    ASFunction *getFunction() const ;

    static ASBlock *createBlock(ASFunction *parent, const std::string& name);

    std::string print(RegMapper *mapper) final;
};

class ASFunction : public ASLabel {
private:
    std::list<ASBlock *> _block_list;
    std::map<void *, int> _stack;
    std::map<Argument *, ASArgument *> _arg_map;
    unsigned _num_args = 0;
    int _sp_pointer = 0;
    bool _has_ret;

    explicit ASFunction(std::string name, unsigned num_args, bool hasRet) : ASLabel(std::move(name)), _num_args(num_args), _has_ret(hasRet) {
        expandNumOperand(num_args);
        for (int i = 0; i < num_args; ++i) {
            setOperand(i, ASArgument::createArgument());
        }
    }

public:
    std::list<ASBlock *> getBlockList();

    void addBlock(ASBlock *block) ;

    void eraseBlock(ASBlock *block) ;
    // 在函数栈空间中分配存储
    int allocStack(void *base, int requiredSize) ;

    // 获取某个栈中的量相对与 sp 指针的偏移量
    int getStackOffset(void *base);
    // 获取偏移量
    int getStackSize() const ;

    int getNumArguments() const ;

    std::vector<ASArgument *> getArguments() ;

    void setArgumentMapping(int idx, Argument *ori) ;

    ASValue *getArgument(int idx) const;
    ASValue *getArgument(Argument *ori);

    static ASFunction *createFunction(std::string name, unsigned i, bool hasRet) ;

    std::string print(RegMapper *mapper) final;

    bool hasReturnValue() const ;
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
    static ASOperand2 *getOperand2(int c);

    static ASOperand2 *getOperand2(ASInstruction *i) ;
    // TODO: 其他 Op2 还没用到，先放着

    Op2Type getOp2Type() const;

    ASValue *getRm() const;

    ASValue *getRs() const;

    std::string print(RegMapper *mapper) final;
};

class ASBinaryInst : public ASInstruction {
private:
    explicit ASBinaryInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASBinaryInst *createASMAdd(ASValue *Rn, ASValue *Op2) ;

    static ASBinaryInst *createASMSub(ASValue *Rn, ASValue *Op2) ;

    static ASBinaryInst *createASMMul(ASValue *Rm, ASValue *Rs) ;

    static ASBinaryInst *createASMDiv(ASValue *Rn, ASValue *Rm) ;

    static ASBinaryInst *createASMAsr(ASValue *Rm, ASValue *Rs) ;

    static ASBinaryInst *createASMLsl(ASValue *Rm, ASValue *Rs) ;

    static ASBinaryInst *createASMLsr(ASValue *Rm, ASValue *Rs) ;

    std::string print(RegMapper *mapper) final;
};

class ASUnaryInst : public ASInstruction {
private:
    explicit ASUnaryInst(ASMInstType ty) : ASInstruction(1, ty) {}

public:
    static ASUnaryInst *createASMMov(ASValue *Op2);
    static ASUnaryInst *createASMMvn(ASValue *Op2);

    std::string print(RegMapper *mapper) final;
};

class ASCmpInst : public ASInstruction {
private:
    explicit ASCmpInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASCmpInst *createASMCmp(ASValue *Rn, ASValue *Op2) ;
    static ASCmpInst *createASMCnz(ASValue *Rn, ASValue *Op2) ;

    std::string print(RegMapper *mapper) final;
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

    ASBranchInst(ASMBranchCond cond, bool link, ASLabel *label) : ASInstruction(1, ASMBrTy), _cond(cond),
                                                                  _withLink(link) {
        setOperand(0, label);
    }

public:
    static ASBranchInst *createBranch(ASLabel *lbl);

    static ASBranchInst *createCondBranch(ASLabel *lbl, ASMBranchCond c) ;

    static ASBranchInst *createLinkBranch(ASLabel *lbl) ;

    static ASBranchInst *createReturnBranch() ;

    ASLabel *getLabel() const;

    bool isLinkBr() const ;

    ASMBranchCond getCondition() const;

    std::string print(RegMapper *mapper) final;
};

class ASPushInst : public ASInstruction {
private:
    ASPushInst() : ASInstruction(0, ASMPushTy) {}

public:
    static ASPushInst *createPush() ;

    std::string print(RegMapper *mapper) final;
};

class ASPopInst : public ASInstruction {
private:
    ASPopInst() : ASInstruction(0, ASMPopTy) {}

public:
    static ASPopInst *createPush();

    std::string print(RegMapper *mapper) final;

};

class ASLoadInst : public ASInstruction {
private:
    explicit ASLoadInst(ASValue *Rn) : ASInstruction(1, ASMLoadTy) {
        setOperand(0, Rn);
    }

    ASLoadInst(ASValue *Rn, ASValue *Op2) : ASInstruction(2, ASMLoadTy) {
        setOperand(0, Rn);
        setOperand(1, Op2);
    }

    bool _isSpOffset = false;
    bool _isLabel = false;


public:
    // ldr r1, =label
    static ASLoadInst *createLoad(ASGlobalValue *gv) ;
    // ldr r1, [r2, r3]
    static ASLoadInst *createLoad(ASInstruction *Rn, ASValue *Op2);
    // ldr r1, [r2]
    static ASLoadInst *createLoad(ASInstruction *Rn) ;
    // ldr r1, [sp, r2]
    static ASLoadInst *createSpLoad(ASValue *Rn) ;
    // ldr r1, [r2, r3]
    static ASLoadInst *createLoad(ASArgument *arg, ASValue *Op2) ;
    bool islabel() ;
    bool isSpOffset() ;


    std::string print(RegMapper *mapper) final;
};

class ASStoreInst : public ASInstruction {
private:
    ASStoreInst(ASValue *Rd, ASValue *Rn) : ASInstruction(2, ASMLoadTy) {
        setOperand(0, Rd);
        setOperand(1, Rn);
    }

    ASStoreInst(ASValue *Rd, ASValue *Rn, ASValue *Op2) : ASInstruction(3, ASMLoadTy) {
        setOperand(0, Rd);
        setOperand(1, Rn);
        setOperand(2, Op2);
    }

    bool _isSp = false;
public:
    // str r1, [r2, r3]
    static ASStoreInst *createStore(ASValue *data, ASInstruction *Rn, ASValue *Op2) ;
    // str r1, [r2]
    static ASStoreInst *createStore(ASValue *data, ASInstruction *Rn) ;
    // str r1, [sp, r2]
    static ASStoreInst *createSpStore(ASValue *data, ASValue *Rn) ;
    // str r1, [r2, r3]
    static ASStoreInst *createStore(ASValue *data, ASArgument *arg, ASValue *Op2);

    std::string print(RegMapper *mapper) final;
    bool isSp() {return _isSp;}

};

class ASPhiInst : public ASInstruction {
private:
    ASPhiInst() : ASInstruction(0, ASMPhiTy) {}

public:
    void addPhiPair(ASBlock *from, ASValue *value) {
        int currentNumOperands = getNumOperands();
        expandNumOperand(2);
        setOperand(currentNumOperands, from);
        setOperand(currentNumOperands + 1, value);
    }

    static ASPhiInst *getPhi() { return new ASPhiInst(); }

    std::vector<std::pair<ASBlock *, ASValue *>> getBBValuePair();

    std::string print(RegMapper *mapper) final;

};

/*
 * 固定的寄存器。这些寄存器无需进行寄存器分配且必须满足要求

class ASFixedRegister : public ASValue {
private:
    explicit ASFixedRegister(int id) : ASValue(0), regId(id) {}

    int regId;
public:
    int getRegID() const { return regId; }

    static ASFixedRegister *getRegister(int regId) { return new ASFixedRegister(regId); }

    std::string print(RegMapper *mapper) final;
};
*/
class ASFunctionCall : public ASInstruction {
private:
    explicit ASFunctionCall(ASFunction *f, const std::vector<ASValue *> &params) : ASInstruction(
            f->getNumArguments() + 1, ASMCallTy) {
        assert(params.size() == f->getNumArguments() && "Param count mismatch");
        setOperand(0, f);
        for (int i = 0; i < f->getNumArguments(); ++i) {
            setOperand(i + 1, params.at(i));
        }
    }

public:
    static ASFunctionCall *getCall(ASFunction *f, const std::vector<ASValue *> &params) ;

    static ASFunctionCall *getCall(ASFunction *f) ;

    ASFunction *getCallee() const ;

    ASValue *getArgument(int idx) const ;

    bool isVoid() const ;

    std::string print(RegMapper *mapper) final;


};

class ASAlloca : public ASValue {
private:
    ASAlloca(int size, int base_sp) : ASValue(0), _sz(size), _base(base_sp) {}
    int _sz;
    int _base;
public:
    static ASAlloca *getAlloca(int size, int base_sp_offset);
    int getSize() const;
    int getBase() const ;
};

class ASReturn : public ASInstruction {
private:
    ASReturn() : ASInstruction(0, ASMRetTy){}
    explicit ASReturn(ASValue *ret) : ASInstruction(1, ASMRetTy) {
        setOperand(0, ret);
    }

public:
    bool isVoid() const ;
    ASValue *getReturnValue() const ;
    static ASReturn *getReturn(ASValue *ret);
    static ASReturn *getReturn();
    std::string print(RegMapper *mapper) final;
};
