//
// Created by 顾超 on 2021/7/30.
//

#include <list>
#include <cassert>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include "IR/Function.h"
#include "ASMIR/RegAllocMapper.h"


class ASValue;

class ASBlock;

class ASFunction;

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
    std::list<ASUse> getUseList() const { return _use; }

    ASValue *getOperand(unsigned idx) const { return _operands[idx]; }

    std::vector<ASValue *> getOperands() const { return _operands; }

    int getNumOperands() const { return _operands.size(); }

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

    virtual std::string print(RegMapper *mapper) { return ""; }

    std::string getName() const { return _name; }

    void setName(std::string n) { _name = std::move(n); }

    void expandNumOperand(unsigned by) { _operands.resize(_operands.size() + by); }
};

class ASArgument : public ASValue {
private:
    ASArgument() : ASValue(0) {}
public:
    static ASArgument *createArgument() { return new ASArgument(); }

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

    int getArraySize() const {
        assert(isArray());
        return _size;
    }

    int getInitialValue() const {
        if (!_initial.empty())
            return _initial[0];
        else
            return 0;
    }

    std::vector<int> getArrayInitial() { return _initial; }

    static ASGlobalValue *create(std::string name, Type *ty, Constant *init = nullptr) {
        // TODO Init
        auto ret = new ASGlobalValue(std::move(name), {});
        if (ty->is_int32_type()) { ret->_array = false; ret->_size = 1; }
        else {
            ret->_array = true;
            int sz = 1;
            auto arr_ty = dynamic_cast<ArrayType *>(ty);
            while (arr_ty) {
                sz *= (int)arr_ty->get_num_of_elements();
                arr_ty = dynamic_cast<ArrayType *>(arr_ty->get_array_element_type());
            }
            ret->_size = sz;
        }
        return ret;
    }

    std::string print(RegMapper *mapper) final;
};

class ASConstant : public ASValue {
    // We don't maintain the use list of ASConstant type
private:
    int _value;

    explicit ASConstant(int val) : ASValue(0), _value(val) {}

public:
    static ASConstant *getConstant(int constVal) { return new ASConstant(constVal); }

    int getValue() const { return _value; }

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
               ty == ASMLslTy || ty == ASMLoadTy || ty == ASMMovTy || ty == ASMMvnTy || ty == ASMAsrTy ||
               ty == ASMPhiTy || ty == ASMCallTy;
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

    std::list<ASInstruction *> getInstList() { return _inst_list; }

    std::list<ASInstruction *> getReverseInstList() {
        std::list<ASInstruction *> ret = _inst_list;
        std::reverse(ret.begin(), ret.end());
        return ret;
    }

    void setParent(ASFunction *f) { _parent = f; }

    ASFunction *getFunction() const { return _parent; }

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

    explicit ASFunction(std::string name, unsigned num_args) : ASLabel(std::move(name)), _num_args(num_args) {
        expandNumOperand(num_args);
        for (int i = 0; i < num_args; ++i) {
            setOperand(i, ASArgument::createArgument());
        }
    }

public:
    std::list<ASBlock *> getBlockList() { return _block_list; }

    void addBlock(ASBlock *block) {
        block->setParent(this);
        _block_list.push_back(block);
    }

    void eraseBlock(ASBlock *block) {
        block->setParent(nullptr);
        _block_list.remove(block);
    }

    // 在函数栈空间中分配存储
    int allocStack(void *base, int requiredSize) {
        int current_sp = _sp_pointer;
        _sp_pointer += requiredSize * 4;
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

    int getNumArguments() const { return _num_args; }

    std::vector<ASArgument *> getArguments() {
        std::vector<ASArgument *> ret;
        for (auto op: getOperands()) {
            ret.push_back(dynamic_cast<ASArgument *>(op));
        }
        return ret;
    }

    void setArgumentMapping(int idx, Argument *ori) { _arg_map[ori] = dynamic_cast<ASArgument *>(getOperand(idx)); }

    ASValue *getArgument(int idx) const { return dynamic_cast<ASArgument *>(getOperand(idx)); }
    ASValue *getArgument(Argument *ori) { return _arg_map.at(ori); }

    static ASFunction *createFunction(std::string name, unsigned i) { return new ASFunction(std::move(name), i); }

    std::string print(RegMapper *mapper) final;
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

    std::string print(RegMapper *mapper) final;
};

class ASBinaryInst : public ASInstruction {
private:
    explicit ASBinaryInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASBinaryInst *createASMAdd(ASValue *Rn, ASValue *Op2) {
        auto ret = new ASBinaryInst(ASMAddTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASBinaryInst *createASMSub(ASValue *Rn, ASValue *Op2) {
        auto ret = new ASBinaryInst(ASMSubTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASBinaryInst *createASMMul(ASValue *Rm, ASValue *Rs) {
        auto ret = new ASBinaryInst(ASMMulTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMDiv(ASValue *Rn, ASValue *Rm) {
        auto ret = new ASBinaryInst(ASMDivTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Rm);
        return ret;
    }

    static ASBinaryInst *createASMAsr(ASValue *Rm, ASValue *Rs) {
        auto ret = new ASBinaryInst(ASMAsrTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMLsl(ASValue *Rm, ASValue *Rs) {
        auto ret = new ASBinaryInst(ASMLslTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    static ASBinaryInst *createASMLsr(ASValue *Rm, ASValue *Rs) {
        auto ret = new ASBinaryInst(ASMLsrTy);
        ret->setOperand(0, Rm);
        ret->setOperand(1, Rs);
        return ret;
    }

    std::string print(RegMapper *mapper) final;
};

class ASUnaryInst : public ASInstruction {
private:
    explicit ASUnaryInst(ASMInstType ty) : ASInstruction(1, ty) {}

public:
    static ASUnaryInst *createASMMov(ASValue *Op2) {
        auto ret = new ASUnaryInst(ASMMovTy);
        ret->setOperand(0, Op2);
        return ret;
    }

    static ASUnaryInst *createASMMvn(ASValue *Op2) {
        auto ret = new ASUnaryInst(ASMMvnTy);
        ret->setOperand(0, Op2);
        return ret;
    }

    std::string print(RegMapper *mapper) final;
};

class ASCmpInst : public ASInstruction {
private:
    explicit ASCmpInst(ASMInstType ty) : ASInstruction(2, ty) {}

public:
    static ASCmpInst *createASMCmp(ASValue *Rn, ASValue *Op2) {
        auto ret = new ASCmpInst(ASMCmpTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

    static ASCmpInst *createASMCnz(ASValue *Rn, ASValue *Op2) {
        auto ret = new ASCmpInst(ASMCmzTy);
        ret->setOperand(0, Rn);
        ret->setOperand(1, Op2);
        return ret;
    }

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
    static ASBranchInst *createBranch(ASLabel *lbl) { return new ASBranchInst(CondNo, false, lbl); }

    static ASBranchInst *createCondBranch(ASLabel *lbl, ASMBranchCond c) { return new ASBranchInst(c, false, lbl); }

    static ASBranchInst *createLinkBranch(ASLabel *lbl) { return new ASBranchInst(CondNo, true, lbl); }

    static ASBranchInst *createReturnBranch() { return new ASBranchInst(CondNo, true, nullptr); } // BX

    ASLabel *getLabel() const { return dynamic_cast<ASLabel *>(getOperand(0)); }

    bool isLinkBr() const { return _withLink; }

    ASMBranchCond getCondition() const { return _cond; }

    std::string print(RegMapper *mapper) final;
};

class ASPushInst : public ASInstruction {
private:
    ASPushInst() : ASInstruction(0, ASMPushTy) {}

public:
    static ASPushInst *createPush() { return new ASPushInst(); }

    std::string print(RegMapper *mapper) final;
};

class ASPopInst : public ASInstruction {
private:
    ASPopInst() : ASInstruction(0, ASMPopTy) {}

public:
    static ASPopInst *createPush() { return new ASPopInst(); }

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
    static ASLoadInst *createLoad(ASGlobalValue *gv) {
        auto ret = new ASLoadInst(gv);
        ret->_isLabel = true;
        return ret;
    }
    // ldr r1, [r2, r3]
    static ASLoadInst *createLoad(ASInstruction *Rn, ASValue *Op2) {
        return new ASLoadInst(Rn, Op2);
    }
    // ldr r1, [r2]
    static ASLoadInst *createLoad(ASInstruction *Rn) {
        return new ASLoadInst(Rn);
    }
    // ldr r1, [sp, r2]
    static ASLoadInst *createSpLoad(ASValue *Rn) {
        auto ret = new ASLoadInst(Rn);
        ret->_isSpOffset = true;
        return ret;
    }
    // ldr r1, [r2, r3]
    static ASLoadInst *createLoad(ASArgument *arg, ASValue *Op2) {
        return new ASLoadInst(arg, Op2);
    }

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
    static ASStoreInst *createStore(ASValue *data, ASInstruction *Rn, ASValue *Op2) {
        return new ASStoreInst(data, Rn, Op2);
    }
    // str r1, [r2]
    static ASStoreInst *createStore(ASValue *data, ASInstruction *Rn) {
        return new ASStoreInst(data, Rn);
    }
    // str r1, [sp, r2]
    static ASStoreInst *createSpStore(ASValue *data, ASValue *Rn) {
        auto ret = new ASStoreInst(data, Rn);
        ret->_isSp = true;
        return ret;
    }
    // str r1, [r2, r3]
    static ASStoreInst *createStore(ASValue *data, ASArgument *arg, ASValue *Op2) {
        return new ASStoreInst(data, arg, Op2);
    }

    std::string print(RegMapper *mapper) final;
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

    std::vector<std::pair<ASBlock *, ASValue *>> getBBValuePair() {
        std::vector<std::pair<ASBlock *, ASValue *>> ret;
        for (int i = 0; i < getNumOperands(); i+=2) {
            ret.emplace_back(dynamic_cast<ASBlock *>(getOperand(i)), getOperand(i + 1));
        }
        return ret;
    }

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
    static ASFunctionCall *getCall(ASFunction *f, const std::vector<ASValue *> &params) {
        return new ASFunctionCall(f, params);
    }

    static ASFunctionCall *getCall(ASFunction *f) {
        return new ASFunctionCall(f, {});
    }

    ASFunction *getCallee() const { return dynamic_cast<ASFunction *>(getOperand(0)); }

    ASValue *getArgument(int idx) const { return getOperand(idx + 1); }

    bool isVoid() const { return getNumOperands() == 1; }

    std::string print(RegMapper *mapper) final;
};

class ASAlloca : public ASValue {
private:
    ASAlloca(int size, int base_sp) : ASValue(0), _sz(size), _base(base_sp) {}
    int _sz;
    int _base;
public:
    static ASAlloca *getAlloca(int size, int base_sp_offset) {
        return new ASAlloca(size, base_sp_offset);
    }

    int getSize() const { return _sz; }
    int getBase() const { return _base; }

};

class ASReturn : public ASInstruction {
private:
    ASReturn() : ASInstruction(0, ASMRetTy){}
    explicit ASReturn(ASValue *ret) : ASInstruction(1, ASMRetTy) {
        setOperand(0, ret);
    }

public:
    bool isVoid() const { return getNumOperands() == 0; }
    ASValue *getReturnValue() const { assert(!isVoid() && "Void return doesn't have a return value."); return getOperand(0); }
    static ASReturn *getReturn(ASValue *ret) { return new ASReturn(ret); }
    static ASReturn *getReturn() { return new ASReturn(); }

    std::string print(RegMapper *mapper) final;
};
