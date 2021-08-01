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
                          {ASInstruction::ASMCmzTy, "cmz"}
                  });


std::string PrintReg(int i) { return " r" + std::to_string(i); }

std::string ASFunctionCall::print(RegMapper *mapper) {
    std::string ret;
    // TODO
    // Step 1: push all active vars in registers into the stack, except the one to store the return value.

    // Step 2: generate br asm code
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    // Step 3: pull all active vars from the stack to the register.
    return ret;
}

std::string ASArgument::print(RegMapper *mapper) {
    // Do nothing
    return ASValue::print(mapper);
}

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

std::string ASConstant::print(RegMapper *mapper) {
    return " #" + std::to_string(getValue());
}

std::string ASBlock::print(RegMapper *mapper) {
    return getName() + ":\n";
}

ASBlock *ASBlock::createBlock(ASFunction *parent, const std::string& name) {
    auto ret = new ASBlock(parent->getName() + "_" + name);
    ret->setParent(parent);
    parent->addBlock(ret);
    return ret;
}


std::string ASFunction::print(RegMapper *mapper) {
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
    ret += "push {r11, lr}\n";
    ret += "add r11, sp, #0\n";
    ret += "sub sp,, sp, #" + std::to_string(getStackSize()) + "\n";

    return ret;
}

std::string ASUnaryInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, this);
    ret += ", ";
    ret += mapper->getName(this, getOperand(0));
    ret += "\n";
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

std::string ASOperand2::print(RegMapper *mapper) {
    return ASValue::print(mapper);
}

std::string ASCmpInst::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += OpNameMap.at(getInstType());
    ret += mapper->getName(this, getOperand(0));
    ret += ", ";
    ret += mapper->getName(this, getOperand(1));
    ret += "\n";
    return ret;
}

std::string ASPushInst::print(RegMapper *mapper) {
    // TODO
    return ASValue::print(mapper);
}

std::string ASPopInst::print(RegMapper *mapper) {
    // TODO
    return ASValue::print(mapper);
}

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

std::string ASReturn::print(RegMapper *mapper) {
    std::string ret = l_spacing;
    ret += "ret ";
    if (!isVoid()) {
        ret += mapper->getName(this, getReturnValue());
    }
    ret += "\n";
    return ret;
}

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