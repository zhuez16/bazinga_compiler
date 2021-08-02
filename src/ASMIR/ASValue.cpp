//
// Created by 顾超 on 2021/7/30.
//

#include "ASMIR/ASValue.hpp"

template<class T>
bool isa(ASValue *inst) { return dynamic_cast<T *>(inst) != nullptr; }



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

bool ASInstruction::hasResult() const {
    auto ty = getInstType();
    return ty == ASMAddTy || ty == ASMSubTy || ty == ASMMulTy || ty == ASMDivTy || ty == ASMLsrTy ||
           ty == ASMLslTy || ty == ASMLoadTy || ty == ASMMovTy || ty == ASMMvnTy || ty == ASMAsrTy ||
           ty == ASMPhiTy || (ty == ASMCallTy && dynamic_cast<const ASFunction *>(getOperand(0))->hasReturnValue());
}

void ASInstruction::setParent(ASBlock *b) { _parent = b; }

ASBlock *ASInstruction::getBlock() const { return _parent; }

ASInstruction::ASMInstType ASInstruction::getInstType() const { return _ty; }

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

std::string ASValue::printUser(RegMapper *mapper)  {
    std::string ret = "    @ User: ";
    for (auto user: getUseList()) {
        ret += mapper->getName(nullptr, user._user);
    }
    ret += "\n";
    return ret;
}

void ASValue::removeUser(unsigned int idx, ASValue *user) {
    ASUse tbd(user, idx);
    for (auto it = _use.begin(); it != _use.end(); ++it) {
        if (*it == tbd) {
            _use.erase(it);
            break;
        }
    }
}

void ASValue::addUser(unsigned int idx, ASValue *user) { _use.emplace_back(user, idx); }

int ASValue::getNumOperands() const { return _operands.size(); }

std::vector<ASValue *> ASValue::getOperands() const { return _operands; }

ASValue *ASValue::getOperand(unsigned int idx) const { return _operands[idx]; }

std::list<ASUse> ASValue::getUseList() const { return _use; }

std::string ASValue::print(RegMapper *mapper) { return ""; }

std::string ASValue::getName() const { return _name; }

void ASValue::setName(std::string n) { _name = std::move(n); }

void ASValue::expandNumOperand(unsigned int by)  { _operands.resize(_operands.size() + by); }

ASAlloca *ASAlloca::getAlloca(int size, int base_sp_offset)  {
        return new ASAlloca(size, base_sp_offset);
    }

int ASAlloca::getSize() const  { return _sz; }

int ASAlloca::getBase() const { return _base; }

