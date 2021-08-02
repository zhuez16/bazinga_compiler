//
// Created by 顾超 on 2021/7/30.
//
#include "ASMIR/ASValue.hpp"


const std::string l_spacing = "    ";
const std::string s_spacing = "  ";
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


std::string generateReg(int i) { return " r" + std::to_string(i); }

void ASFunctionCall::generate(RegMapper *mapper) {
    std::string ret = l_spacing;
    // TODO
    // Step 1: push all active vars in registers into the stack, except the one to store the return value.

    // Step 2: generate br asm code
    ret += OpNameMap.at(getInstType());
    // TODO: remove this if function doesn't have a return value
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    for (int i = 1; i < getNumOperands(); ++i) {
        ret += ", ";
        ret += mapper->getName(this, getOperand(i));
    }
    ret += "\n";
    this->getBlock()->addInstruction(ret);
    // Step 3: pull all active vars from the stack to the register.
}

void ASArgument::generate(RegMapper *mapper) {
    // Do nothing
    return ASValue::generate(mapper);
}
//Please use ASGlobalValue::print to directly generate the asm code of a GlobalValue.
//void ASGlobalValue::generate(RegMapper *mapper) {
//    /**  Global value header
//     *          .global t
//     *          .data
//     *          .align  2
//     *          .type   t, %object
//     *          .size   t, 40
//     *  t:
//     *          .word   0
//     *          .word   1
//     *          .space  32
//     */
//    std::string ret = l_spacing + ".global\t" + getName() + '\n';
//    ret += l_spacing + ".align\t2\n";
//    ret += l_spacing + ".type\t" + getName() + ", %object";
//    ret += l_spacing + ".size\t" + getName() + ", " + (isArray() ? std::to_string(getArraySize() * 4) : "4") + '\n';
//    ret += getName() + '\n';
//    if (isArray()) {
//        for (auto i: getArrayInitial()) {
//            ret += l_spacing;
//            ret += ".word";
//            ret += s_spacing;
//            ret += std::to_string(i);
//            ret += '\n';
//        }
//        // Uninitialized items
//        int num_un = getArraySize() - (int) getArrayInitial().size();
//        if (num_un > 0) {
//            ret += l_spacing;
//            ret += ".space";
//            ret += s_spacing;
//            ret += std::to_string(4 * num_un);
//            ret += '\n';
//        }
//    } else {
//        ret += l_spacing;
//        ret += ".word " + std::to_string(getInitialValue()) + '\n';
//    }
//}

void ASConstant::generate(RegMapper *mapper) {
}

void ASBlock::generate(RegMapper *mapper){
    std::vector<ASInstruction *> phis;
    for (auto *instr:this->getInstList()){
        if (instr->getInstType() != ASInstruction::ASMPhiTy)
            instr->generate(mapper);
        else
            phis.push_back(instr);
    }
    for (auto *instr:phis){
        instr->generate(mapper);
    }
}

void ASFunction::generate(RegMapper *mapper) {
    // TODO: Function header

    /*
     * push   {r11, lr}
     * add    r11, sp, #0
     * sub    sp, sp, #16
     * */
    auto ret = "ASFunction: " + getName() + ":\n";
    ret += s_spacing + "@ Alloca Stack Required: " + std::to_string(getStackSize()) + '\n';
    ret += s_spacing + "@ Arguments:\n";
    for (auto arg: getArguments()) {
        ret += l_spacing + "@Arg: " + mapper->getName(nullptr, arg) + "\n";
    }
    std::vector<int> saved_registers;

    ret += l_spacing + "push {r11, lr" + "}\n";
    ret += l_spacing + "add r11, sp, #0\n";
    ret += l_spacing + "sub sp, sp, #" + std::to_string(getStackSize()) + "\n";
    for (auto *bb:this->getBlockList()){
        bb->generate(mapper);
    }
    // assume that ActiveVars stores the active regs in the function;
    ret += l_spacing + "add sp, r11, #0\n";
    ret += l_spacing + "pop {r11, lr" + "}\n";

}

void ASAlloca::generate(RegMapper *mapper){}
void ASGlobalValue::generate(RegMapper *mapper){}
void ASUnaryInst::generate(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += "\n";
    this->getBlock()->addInstruction(ret);
}

void ASBinaryInst::generate(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    this->getBlock()->addInstruction(ret);
}

void ASOperand2::generate(RegMapper *mapper) {
    return ASValue::generate(mapper);
}

void ASCmpInst::generate(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    this->getBlock()->addInstruction(ret);
}

void ASPushInst::generate(RegMapper *mapper) {
    // TODO
    return ASValue::generate(mapper);
}

void ASPopInst::generate(RegMapper *mapper) {
    // TODO
    return ASValue::generate(mapper);
}

void ASPhiInst::generate(RegMapper *mapper) {
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
    auto target=mapper->getRegister(this,this);
    for (auto operand:this->getOperands()) {
        auto temp_bb = dynamic_cast<ASBlock *> (operand);
        auto temp_br_inst = temp_bb->getInstList().back();
        auto reg=mapper->getRegister(this,operand);
        temp_bb->getInstList().pop_back();
        //TODO How to generate a instruction with a specific register?
        temp_bb->addInstruction("mov "+std::to_string(target)+","+std::to_string(reg)+"\n");
        temp_bb->addInstruction(temp_br_inst);
    }
}

void ASReturn::generate(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += "ret ";
    if (!isVoid()) {
        ret += mapper->getName(this, getReturnValue());
    }
    ret += "\n";
    this->getBlock()->addInstruction(ret);
}

void ASBranchInst::generate(RegMapper *mapper) {
    if (isLinkBr()) {
        this->getBlock()->addInstruction("bx lr\n");
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
    this->getBlock()->addInstruction(ret);
}

void ASLoadInst::generate(RegMapper *mapper) {
    auto ret = l_spacing + "ldr " + mapper->getName(this, this) + ", ";
    if (_isLabel) {
        this->getBlock()->addInstruction( ret + "=" + getOperand(0)->getName() + "\n");
        return;
    }
    if (_isSpOffset) {
        this->getBlock()->addInstruction(ret + "[sp, " + mapper->getName(this, getOperand(0)) + "]\n");
        return;
    }
    if (getNumOperands() == 1) {
        this->getBlock()->addInstruction( ret + "[" + mapper->getName(this, getOperand(0)) + "]\n");
        return;
    } else {
        this->getBlock()->addInstruction(
         ret + "[" + mapper->getName(this, getOperand(0)) + ", "
               + mapper->getName(this, getOperand(1)) +"]\n");
        return;
    }
}

void ASStoreInst::generate(RegMapper *mapper) {
    auto ret = l_spacing + "str " + mapper->getName(this, getOperand(0)) + ", ";
    if (_isSp) {
        this->getBlock()->addInstruction( ret + "[sp, " + mapper->getName(this, getOperand(1)) + "]\n");
        return;
    }
    if (getNumOperands() == 2) {
        this->getBlock()->addInstruction(ret + "[" + mapper->getName(this, getOperand(1)) + "]\n");
        return;
    } else {
        this->getBlock()->addInstruction(ret + "[" + mapper->getName(this, getOperand(1)) + ", "
               + mapper->getName(this, getOperand(2)) +"]\n");
        return;
    }
    this->getBlock()->addInstruction(ret);
}