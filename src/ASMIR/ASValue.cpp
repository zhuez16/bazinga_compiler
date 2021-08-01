//
// Created by 顾超 on 2021/7/30.
//

#include "ASMIR/ASValue.hpp"

template<class T>
bool isa(ASValue *inst) { return dynamic_cast<T *>(inst) == nullptr; }



void ASValue::setOperand(unsigned idx, ASValue *v) {
    if (auto inst = dynamic_cast<ASInstruction *>(v)) {
        assert(inst->getInstType() == ASInstruction::ASMCallTy || inst->hasResult() && "Only an instruction with a result can be used as operand.");
    }
    // 单独处理Operand 2
    if (auto op2 = dynamic_cast<ASOperand2 *>(v)) {
        if (auto rm = op2->getRm()) {
            if (!isa<ASLabel>(rm) && !isa<ASConstant>(rm)) {
                rm->addUser(idx, this);
            }
        }
        if (auto rs = op2->getRs()) {
            if (!isa<ASLabel>(rs) && !isa<ASConstant>(rs)) {
                rs->addUser(idx, this);
            }
        }
    }
        // 我们不维护Label与Constant的引用关系
    else if (!isa<ASLabel>(v) && !isa<ASConstant>(v)) {
        v->addUser(idx, this);
    }
    _operands[idx] = v;
}

std::vector<ASValue *> ASValue::getOperandsWithOp2() {
    std::vector<ASValue *> val;
    for (auto i : getOperands()) {
        if (auto op2 = dynamic_cast<ASOperand2 *>(i)) {
            if (op2->getRm()) {
                val.push_back(op2->getRm());
            }
            if (op2->getRs()) {
                val.push_back(op2->getRs());
            }
        } else {
            val.push_back(op2);
        }
    }
    return val;
}


