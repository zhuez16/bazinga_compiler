//
// Created by 顾超 on 2021/7/30.
//
//#include "ASMIR/ASValue.hpp"

#include "ASMIR/RegAllocMapper.h"

const std::string l_spacing = "    ";
const std::string s_spacing = "  ";
const std::string temp_reg="r12";
const std::map<ASInstruction::ASMInstType, std::string>
        OpNameMap({
                          {ASInstruction::ASMMovTy, "mov"},
                          {ASInstruction::ASMMvnTy, "mvn"},
                          {ASInstruction::ASMAddTy, "add"},
                          {ASInstruction::ASMSubTy, "sub"},
                          {ASInstruction::ASMMulTy, "mul"},
                          {ASInstruction::ASMDivTy, "div"},
                          {ASInstruction::ASMLoadTy, "ldr"},
                          {ASInstruction::ASMStoreTy, "str"},
                          {ASInstruction::ASMBrTy, "b"},
                          {ASInstruction::ASMCmpTy, "cmp"},
                          {ASInstruction::ASMCmzTy, "cmz"},
                          {ASInstruction::ASMCallTy, "call"},
                          {ASInstruction::ASMLslTy, "lsl"},
                          {ASInstruction::ASMLsrTy, "lsr"}
                  });


std::string PrintReg(int i) { return " r" + std::to_string(i); }

std::string ASFunctionCall::print(RegMapper *mapper) {
    std::string ret = "";
    // TODO
    // Step 1: push all operands into the regs and the stack
    std::map<int, int> reg_in, reg_out;
    int i=0;
    for (auto op:getOperands()){
        if (dynamic_cast<ASFunction*>(op)) continue;
        if (dynamic_cast<ASConstant*>(op)){
            if (i>=4){
                ret+="    mov r0,#"+std::to_string(dynamic_cast<ASConstant*>(op)->getValue())+"\n";
                ret+="    str r0,[sp,#"+std::to_string((i-4)*4)+"]\n";
            }
        }
        else{
            if (i>=4){
                ret+="    str "+mapper->getName(this,op)+",[sp,#"+std::to_string((i-4)*4)+"]\n";
            }
        }
        i++;
    }
    if (getNumOperands()>4) ret+="    add sp,sp,#"+std::to_string((getNumOperands()-5)*4)+"\n";
    i=0;
    for (auto op:getOperands()){
        if (dynamic_cast<ASFunction*>(op)) continue;
        if (dynamic_cast<ASConstant*>(op)){
            if (i>=4){
                break;
            }
            ret+="    mov r"+std::to_string(i)+",#"+std::to_string(dynamic_cast<ASConstant *>(op)->getValue())+"\n";
            reg_in[i]=i;
            reg_out[i]=i;
        }
        else{
            if (i<4){
                reg_in[mapper->getRegister(this,op)]=i;
                reg_out[i]=mapper->getRegister(this,op);
            }
            else{
                break;
            }
        }
        i++;
    }
    int num_of_op=getNumOperands()-1;
    while (num_of_op){
        std::stack<int> sta;
        int temp=getNumOperands()-2;
        while (!reg_in.count(temp) || reg_in.at(temp)<0) temp--;
        if (reg_in[temp]==temp){
            num_of_op--;
            continue;
        }
        sta.push(temp);
        while (sta.top() < getNumOperands()-1){
            int flag=sta.top();
            if (!reg_in.count(sta.top()))break;
            int next=reg_in[sta.top()];
            if (next==temp) break;
            sta.push(next);
        }
        int next=sta.top();
        num_of_op-=sta.size();
        if (next==temp){
            ret+="    mov r12,"+std::to_string(temp)+"\n";
            while (sta.size()>1){
                ret+="    mov r"+std::to_string(reg_in[sta.top()])+",r"+std::to_string(sta.top());
                reg_in[sta.top()]=-1;
                sta.pop();
            }
            ret+="    mov r"+std::to_string(sta.top())+",r12\n";
            reg_in[sta.top()]=-1;
            sta.pop();
        }
        else{
            sta.pop();
            while (!sta.empty()){
                ret+="mov r"+std::to_string(reg_in[sta.top()])+",r"+std::to_string(sta.top())+"\n";
                reg_in[sta.top()]=-1;
                sta.pop();
            }
        }
    }
    for (int j=0;j<i;j++){
        if (reg_out[j]>=i)
            ret+="    mov r"+std::to_string(j)+",r"+std::to_string(reg_out[j])+"\n";
    }
    // Step 2: generate br asm code
    //ret += l_spacing+OpNameMap.at(getInstType());
    ret += "    bl "+this->getOperand(0)->getName()+"\n";
    // TODO: remove this if function doesn't have a return value

    ret += "    mov "+mapper->getName(this,this)+","+"r0\n";
    if (getNumOperands()>4) ret+="    sub sp,sp,#"+std::to_string((getNumOperands()-5)*4)+"\n";
    return ret;
}

ASFunctionCall *ASFunctionCall::getCall(ASFunction *f, const std::vector<ASValue *> &params) {
    return new ASFunctionCall(f, params);
}

ASFunctionCall *ASFunctionCall::getCall(ASFunction *f) {
    return new ASFunctionCall(f, {});
}

ASFunction *ASFunctionCall::getCallee() const { return dynamic_cast<ASFunction *>(getOperand(0)); }

ASValue *ASFunctionCall::getArgument(int idx) const { return getOperand(idx + 1); }

bool ASFunctionCall::isVoid() const { return getNumOperands() == 1; }

std::string ASArgument::print(RegMapper *mapper) {
    // Do nothing
    return ASValue::print(mapper);
}

ASArgument *ASArgument::createArgument() { return new ASArgument(); }

std::string ASGlobalValue::print(RegMapper *mapper) {
    /**  Global value header
     *          .global t
     *          .data
     *          .align  2
     *          .type   t, %object
     *          .size   t, 40
     *  t:
     *          .word   0
     *          .word   1
     *          .space  32
     */
    std::string ret = l_spacing + ".global\t" + getName() + '\n';
    ret += l_spacing + ".align\t2\n";
    ret += l_spacing + ".type\t" + getName() + ", %object";
    ret += l_spacing + ".size\t" + getName() + ", " + (isArray() ? std::to_string(getArraySize() * 4) : "4") + '\n';
    ret += getName() + '\n';
    if (isArray()) {
        for (auto i: getArrayInitial()) {
            ret += l_spacing;
            ret += ".word";
            ret += s_spacing;
            ret += std::to_string(i);
            ret += '\n';
        }
        // Uninitialized items
        int num_un = getArraySize() - (int) getArrayInitial().size();
        if (num_un > 0) {
            ret += l_spacing;
            ret += ".space";
            ret += s_spacing;
            ret += std::to_string(4 * num_un);
            ret += '\n';
        }
    } else {
        ret += l_spacing;
        ret += ".word " + std::to_string(getInitialValue()) + '\n';
    }
    return ret;
}

int ASGlobalValue::getArraySize() const {
    assert(isArray());
    return _size;
}

int ASGlobalValue::getInitialValue() const {
    if (!_initial.empty())
        return _initial[0];
    else
        return 0;
}

ASGlobalValue *ASGlobalValue::create(std::string name, Type *ty, Constant *init){
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

std::vector<int> ASGlobalValue::getArrayInitial()  { return _initial; }


std::string ASConstant::print(RegMapper *mapper) {
    return " #" + std::to_string(getValue());
}


int ASConstant::getValue()  const { return _value; }

ASConstant *ASConstant::getConstant(int constVal) { return new ASConstant(constVal); }

std::string ASBlock::print(RegMapper *mapper) {
    return getName() + ":\n";
}

ASBlock *ASBlock::createBlock(ASFunction *parent, const std::string& name) {
    auto ret = new ASBlock(parent->getName() + "_" + name);
    ret->setParent(parent);
    parent->addBlock(ret);
    return ret;
}

std::list<std::string> ASBlock::get_asm_inst() {return _inst_print;}

void ASBlock::addInstruction(ASInstruction *inst){
    inst->setParent(this);
    _inst_list.push_back(inst);
}

void ASBlock::addInstruction(std::string inst) {
    _inst_print.push_back(inst);
}

std::list<ASInstruction *> &ASBlock::getInstList()  { return _inst_list; }

std::list<ASInstruction *> ASBlock::getReverseInstList()  {
    std::list<ASInstruction *> ret = _inst_list;
    std::reverse(ret.begin(), ret.end());
    return ret;
}

void ASBlock::setParent(ASFunction *f){ _parent = f; }

ASFunction *ASBlock::getFunction() const { return _parent; }

std::list<std::string> &ASBlock::get_inst_print() {
    return _inst_print;
}

std::string ASFunction::print(RegMapper *mapper) {
    // TODO: Function header

    /*
     * push   {r11, lr}
     * add    r11, sp, #0
     * sub    sp, sp, #16
     * */
    auto ret = getName() + ":\n";
    ret += s_spacing + "@ Alloca Stack Required: " + std::to_string(getStackSize()) + '\n';
    ret += s_spacing + "@ Arguments:\n";
//    for (auto arg: getArguments()) {
//        ret += l_spacing + "@Arg: " + mapper->getName(nullptr, arg) + "\n";
//    }
    auto ssa_mapper=dynamic_cast<SsaRegMapper *> (mapper);
    if (ssa_mapper != nullptr){
        std::vector<int> saved_register;
        std::map<int, bool> saved_register_map;
        bool has_call=false;
        for (auto bb:getBlockList()){
            for (auto instr:bb->getInstList()){
                if (instr->getInstType()==ASInstruction::ASMCallTy) has_call=true;
                int reg=mapper->getRegister(instr,instr);
                if (!saved_register_map.count(reg)){
                    if (std::min(getNumArguments(),4) <= reg && reg < 11){
                        saved_register_map[reg]=true;
                        saved_register.push_back(reg);
                    }
                }
            }
        }
        ret += l_spacing + "push {";
        for (auto reg:saved_register){
            ret+="r"+std::to_string(reg)+",";
        }
        if (has_call) ret += "r11,lr}\n";
        else ret += "r11}\n";
        ret += l_spacing + "add r11,sp,#0\n";
        ret += l_spacing + "sub sp,sp,#" + std::to_string(getStackSize()) + "\n";
        this->pushed_offset=saved_register.size()+has_call+1;
        return ret;
    }
    ret += l_spacing + "push {r11, lr}\n";
    ret += l_spacing + "add r11, sp, #0\n";
    ret += l_spacing + "sub sp, sp, #" + std::to_string(getStackSize()) + "\n";

    return ret;
}

std::list<ASBlock *> ASFunction::getBlockList()  { return _block_list; }

void ASFunction::addBlock(ASBlock *block) {
    block->setParent(this);
    _block_list.push_back(block);
}

void ASFunction::eraseBlock(ASBlock *block) {
    block->setParent(nullptr);
    _block_list.remove(block);
}

int ASFunction::allocStack(void *base, int requiredSize) {
    int current_sp = _sp_pointer;
    _sp_pointer += requiredSize * 4;
    _stack[base] = current_sp;
    return current_sp;
}

int ASFunction::getStackOffset(void *base)  {
    if (_stack.find(base) == _stack.end()) return -1;
    return _stack[base];
}

std::vector<ASArgument *> ASFunction::getArguments(){
    std::vector<ASArgument *> ret;
    for (auto op: getOperands()) {
        ret.push_back(dynamic_cast<ASArgument *>(op));
    }
    return ret;
}

int ASFunction::getNumArguments() const { return _num_args; }

int ASFunction::getStackSize() const { return _sp_pointer; }

void ASFunction::setArgumentMapping(int idx, Argument *ori) { _arg_map[ori] = dynamic_cast<ASArgument *>(getOperand(idx)); }

ASValue *ASFunction::getArgument(int idx) const  { return dynamic_cast<ASArgument *>(getOperand(idx)); }

ASValue *ASFunction::getArgument(Argument *ori) { return _arg_map.at(ori); }

ASFunction *ASFunction::createFunction(std::string name, unsigned int i, bool hasRet) { return new ASFunction(std::move(name), i, hasRet); }

bool ASFunction::hasReturnValue() const { return _has_ret; }

std::string ASUnaryInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += "\n";
    return ret;
}

ASUnaryInst *ASUnaryInst::createASMMov(ASValue *Op2)  {
    auto ret = new ASUnaryInst(ASMMovTy);
    ret->setOperand(0, Op2);
    return ret;
}

ASUnaryInst *ASUnaryInst::createASMMvn(ASValue *Op2) {
    auto ret = new ASUnaryInst(ASMMvnTy);
    ret->setOperand(0, Op2);
    return ret;
}

std::string ASBinaryInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMAdd(ASValue *Rn, ASValue *Op2) {
    auto ret = new ASBinaryInst(ASMAddTy);
    ret->setOperand(0, Rn);
    ret->setOperand(1, Op2);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMSub(ASValue *Rn, ASValue *Op2) {
    auto ret = new ASBinaryInst(ASMSubTy);
    ret->setOperand(0, Rn);
    ret->setOperand(1, Op2);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMMul(ASValue *Rm, ASValue *Rs) {
    auto ret = new ASBinaryInst(ASMMulTy);
    ret->setOperand(0, Rm);
    ret->setOperand(1, Rs);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMDiv(ASValue *Rn, ASValue *Rm) {
    auto ret = new ASBinaryInst(ASMDivTy);
    ret->setOperand(0, Rn);
    ret->setOperand(1, Rm);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMAsr(ASValue *Rm, ASValue *Rs) {
    auto ret = new ASBinaryInst(ASMAsrTy);
    ret->setOperand(0, Rm);
    ret->setOperand(1, Rs);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMLsl(ASValue *Rm, ASValue *Rs) {
    auto ret = new ASBinaryInst(ASMLslTy);
    ret->setOperand(0, Rm);
    ret->setOperand(1, Rs);
    return ret;
}

ASBinaryInst *ASBinaryInst::createASMLsr(ASValue *Rm, ASValue *Rs) {
    auto ret = new ASBinaryInst(ASMLsrTy);
    ret->setOperand(0, Rm);
    ret->setOperand(1, Rs);
    return ret;
}

std::string ASOperand2::print(RegMapper *mapper) {
    return ASValue::print(mapper);
}

ASOperand2 *ASOperand2::getOperand2(int c)  {
    auto ret = new ASOperand2(Op2ImmTy);
    ret->base = ASConstant::getConstant(c);
    return ret;
}

ASOperand2 *ASOperand2::getOperand2(ASInstruction *i) {
    auto ret = new ASOperand2(Op2RegTy);
    ret->base = i;
    return ret;
}

ASOperand2::Op2Type ASOperand2::getOp2Type() const  { return _ty; }

ASValue *ASOperand2::getRm() const  { return base; }

ASValue *ASOperand2::getRs() const  { return shift; }


std::string ASCmpInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    return ret;
}

ASCmpInst *ASCmpInst::createASMCmp(ASValue *Rn, ASValue *Op2) {
    auto ret = new ASCmpInst(ASMCmpTy);
    ret->setOperand(0, Rn);
    ret->setOperand(1, Op2);
    return ret;
}

ASCmpInst *ASCmpInst::createASMCnz(ASValue *Rn, ASValue *Op2) {
    auto ret = new ASCmpInst(ASMCmzTy);
    ret->setOperand(0, Rn);
    ret->setOperand(1, Op2);
    return ret;
}

std::string ASPushInst::print(RegMapper *mapper) {
    // TODO
    return ASValue::print(mapper);
}

ASPushInst *ASPushInst::createPush() { return new ASPushInst(); }

std::string ASPopInst::print(RegMapper *mapper) {
    // TODO
    return ASValue::print(mapper);
}

ASPopInst *ASPopInst::createPush()  { return new ASPopInst(); }

std::string ASPhiInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += "phi ";
    ret += mapper->getName(this, this);
    ret += ", ";
    for (auto p: getBBValuePair()) {
        ret += "[" + p.first->getName() + ", " + mapper->getName(this, p.second) + "], ";
    }
    ret.pop_back();
    ret.pop_back();
    ret += "\n";

    return ret;
}

std::vector<std::pair<ASBlock *, ASValue *>> ASPhiInst::getBBValuePair()  {
    std::vector<std::pair<ASBlock *, ASValue *>> ret;
    for (int i = 0; i < getNumOperands(); i+=2) {
        ret.emplace_back(dynamic_cast<ASBlock *>(getOperand(i)), getOperand(i + 1));
    }
    return ret;
}
std::string ASReturn::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += "mov r0," + mapper->getName(this,this->getOperand(0))+"\n";
    ret += l_spacing + "b "+this->getBlock()->getFunction()->getName()+"_"+this->getBlock()->getFunction()->getName()+"_Exit\n";
    return ret;
}

bool ASReturn::isVoid() const { return getNumOperands() == 0; }

ASValue *ASReturn::getReturnValue() const { assert(!isVoid() && "Void return doesn't have a return value."); return getOperand(0); }

ASReturn *ASReturn::getReturn(ASValue *ret) { return new ASReturn(ret); }

ASReturn *ASReturn::getReturn() { return new ASReturn(); }
std::string ASBranchInst::print(RegMapper *mapper) {
    if (isLinkBr()) {
        return l_spacing + "bx lr\n";
    }
    std::string ret = l_spacing;
    switch (getCondition()) {
        case CondNo:
            ret += "b";
            break;
        case CondEQ:
            ret += "beq";
            break;
        case CondNE:
            ret += "bne";
            break;
        case CondLT:
            ret += "blt";
            break;
        case CondLE:
            ret += "ble";
            break;
        case CondGT:
            ret += "bgt";
            break;
        case CondGE:
            ret += "bge";
            break;
    }
    ret += " ";
    ret += mapper->getName(this, getLabel());
    ret += "\n";
    return ret;
}

ASBranchInst *ASBranchInst::createBranch(ASLabel *lbl)  { return new ASBranchInst(CondNo, false, lbl); }

ASBranchInst *ASBranchInst::createCondBranch(ASLabel *lbl, ASBranchInst::ASMBranchCond c) { return new ASBranchInst(c, false, lbl); }

ASBranchInst *ASBranchInst::createLinkBranch(ASLabel *lbl) { return new ASBranchInst(CondNo, true, lbl); }

ASBranchInst *ASBranchInst::createReturnBranch() { return new ASBranchInst(CondNo, true, nullptr); }

ASLabel *ASBranchInst::getLabel() const  { return dynamic_cast<ASLabel *>(getOperand(0)); }

bool ASBranchInst::isLinkBr() const { return _withLink; }

ASBranchInst::ASMBranchCond ASBranchInst::getCondition() const  { return _cond; }

// BX
std::string ASLoadInst::print(RegMapper *mapper) {
    auto ret = l_spacing + "ldr " + mapper->getName(this, this) + ", ";
    if (_isLabel) {
        return ret + "=" + getOperand(0)->getName() + "\n";
    }
    if (_isSpOffset) {
        return ret + "[sp, " + mapper->getName(this, getOperand(0)) + "]\n";
    }
    if (getNumOperands() == 1) {
        return ret + "[" + mapper->getName(this, getOperand(0)) + "]\n";
    } else {
        return ret + "[" + mapper->getName(this, getOperand(0)) + ", "
               + mapper->getName(this, getOperand(1)) +"]\n";
    }
}

ASLoadInst *ASLoadInst::createLoad(ASGlobalValue *gv) {
    auto ret = new ASLoadInst(gv);
    ret->_isLabel = true;
    return ret;
}

ASLoadInst *ASLoadInst::createLoad(ASInstruction *Rn, ASValue *Op2)  {
    return new ASLoadInst(Rn, Op2);
}

ASLoadInst *ASLoadInst::createLoad(ASInstruction *Rn) {
    return new ASLoadInst(Rn);
}

ASLoadInst *ASLoadInst::createSpLoad(ASValue *Rn) {
    auto ret = new ASLoadInst(Rn);
    ret->_isSpOffset = true;
    return ret;
}

ASLoadInst *ASLoadInst::createLoad(ASArgument *arg, ASValue *Op2){
    return new ASLoadInst(arg, Op2);
}

bool ASLoadInst::islabel() {return _isLabel;}

bool ASLoadInst::isSpOffset() {return _isSpOffset;}
std::string ASStoreInst::print(RegMapper *mapper) {
    auto ret = l_spacing + "str " + mapper->getName(this, getOperand(0)) + ", ";
    if (_isSp) {
        return ret + "[sp, " + mapper->getName(this, getOperand(1)) + "]\n";
    }
    if (getNumOperands() == 2) {
        return ret + "[" + mapper->getName(this, getOperand(1)) + "]\n";
    } else {
        return ret + "[" + mapper->getName(this, getOperand(1)) + ", "
               + mapper->getName(this, getOperand(2)) +"]\n";
    }
}

ASStoreInst *ASStoreInst::createStore(ASValue *data, ASInstruction *Rn, ASValue *Op2) {
    return new ASStoreInst(data, Rn, Op2);
}

ASStoreInst *ASStoreInst::createStore(ASValue *data, ASInstruction *Rn) {
    return new ASStoreInst(data, Rn);
}

ASStoreInst *ASStoreInst::createSpStore(ASValue *data, ASValue *Rn) {
    auto ret = new ASStoreInst(data, Rn);
    ret->_isSp = true;
    return ret;
}

ASStoreInst *ASStoreInst::createStore(ASValue *data, ASArgument *arg, ASValue *Op2)  {
    return new ASStoreInst(data, arg, Op2);
}

