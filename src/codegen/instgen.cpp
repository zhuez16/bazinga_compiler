//
// Created by mskhana on 2021/7/21.
//

#include "include/codegen/instgen.h"

#include <cmath>
#include <tuple>

namespace InstGen {

std::string condCode(const CmpOp &cond) {
    std::string code;
    switch (cond) {
        case EQ:
            code += "eq";
            break;
        case NE:
            code += "ne";
            break;
        case GT:
            code += "gt";
            break;
        case GE:
            code += "ge";
            break;
        case LT:
            code += "lt";
            break;
        case LE:
            code += "le";
            break;
        default:
            break;
    }
    return code;
}

std::string gen_push(const std::vector<Reg> &reg_list) {
    std::string code;
    bool flag = false;
    code += tab;
    code += "push";
    code += " ";
    code += "{";
    for (auto &i : reg_list) {
        if (flag) 
            code += ", ";
        code += i.getName();
        flag = true;
    }
    code += "}";
    code += newline;
    return code;
}

std::string gen_pop(const std::vector<Reg> &reg_list) {
    std::string code;
    bool flag = false;
    code += tab;
    code += "pop";
    code += " ";
    code += "{";
    for (auto &i : reg_list) {
        if (flag) 
            code += ", ";
        code += i.getName();
        flag = true;
    }
    code += "}";
    code += newline;
    return code;
}

std::string gen_mov(const Reg &target, const Value &source, const CmpOp &cond) {
    std::string code;
    if (source.is_reg() && target.getName() == source.getName())
        return code;
    code += tab;
    code += "mov";
    code += condCode(cond);
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_mvn(const Reg &target, const Value &source, const CmpOp &cond) {
    std::string code;
    code += tab;
    code += "mvn";
    code += condCode(cond);
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_movw(const Reg &target, const Value &source, const CmpOp &cond) {
    std::string code;
    code += tab;
    code += "movw";
    code += condCode(cond);
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_movt(const Reg &target, const Value &source, const CmpOp &cond) {
    std::string code;
    code += tab;
    code += "movt";
    code += condCode(cond);
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

/*
std::string setValue(const Reg &target, const Constant &source) {
    std::string code;
    auto val = source.getValue();
    if (0 <= val && val <= imm_16_max)
        code += mov(target, Constant(val));
    else if (-imm_8_max <= val && val <= 0)
        code += mvn(target, Constant(-val - 1));
    else {
        uint32_t imm = source.getValue();
        uint32_t imm_low = imm & ((1 << 16) - 1);
        uint32_t imm_high = imm >> 16;
        code += tab;
        code += "movw";
        code += " ";
        code += target.getName();
        code += ", ";
        code += "#" + std::to_string(imm_low);
        code += newline;
        code += tab;
        code += "movt";
        code += " ";
        code += target.getName();
        code += ", ";
        code += "#" + std::to_string(imm_high);
        code += newline;
    }
    return code;
}
*/

std::string gen_adrl(const Reg &target, const Label &source) {
    std::string code;
    code += tab;
    code += "adrl";
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_ldr(const Reg &target, const Addr &source) {
    std::string code;
    code += tab;
    code += "ldr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_ldr(const Reg &target, const Label &source) {
    std::string code;
    code += tab;
    code += "ldr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += source.getName();
    code += newline;
    return code;
}

std::string gen_ldr(const Reg &target, const Reg &base, const Reg &offset) {
    return ldr(target, base, offset, Constant(0));
}

std::string gen_ldr(const Reg &target, const Reg &base, const Reg &offset,
                const Constant &shift) {
    std::string code;
    code += tab;
    code += "ldr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += "[";
    code += base.getName();
    code += ", ";
    code += "+";
    code += offset.getName();
    code += ", ";
    code += "lsl";
    code += " ";
    code += shift.getName();
    code += "]";
    code += newline;
    return code;
}

std::string gen_str(const Reg &source, const Addr &target) {
    std::string code;
    code += tab;
    code += "str";
    code += " ";
    code += source.getName();
    code += ", ";
    code += target.getName();
    code += newline;
    return code;
}

std::string gen_str(const Reg &source, const Label &target) {
    std::string code;
    code += spaces;
    code += "str";
    code += " ";
    code += source.getName();
    code += ", ";
    code += target.getName();
    code += newline;
    return code;
}

std::string gen_str(const Reg &target, const Reg &base, const Reg &offset) {
    return gen_str(target, base, offset, Constant(0));
}

std::string gen_str(const Reg &target, const Reg &base, const Reg &offset,
                const Constant &shift) {
    std::string code;
    code += tab;
    code += "str";
    code += " ";
    code += target.getName();
    code += ", ";
    code += "[";
    code += base.getName();
    code += ", ";
    code += "+";
    code += offset.getName();
    code += ", ";
    code += "lsl";
    code += " ";
    code += shift.getName();
    code += "]";
    code += newline;
    return code;
}

std::string gen_bl(const std::string &target_func) {
    std::string code;
    code += tab;
    code += "bl";
    code += " ";
    code += target_func;
    code += newline;
    return code;
}

std::string gen_add(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "add";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_sub(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "sub";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_rsb(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "rsb";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_and(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "and";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_orr(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "orr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_eor(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "eor";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_clz(const Reg &target, const Reg &op1) {
    std::string code;
    code += tab;
    code += "clz";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += newline;
    return code;
}

std::string gen_lsl(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "lsl";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_asl(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "asl";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_lsr(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "lsr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_asr(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "asr";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_mul(const Reg &target, const Reg &op1, const Reg &op2) {
    std::string code;
    code += tab;
    code += "mul";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_smmul(const Reg &target, const Reg &op1, const Reg &op2) {
    std::string code;
    code += tab;
    code += "smmul";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_mla(const Reg &target, const Reg &op1, const Reg &op2,
                const Reg &op3) {
    std::string code;
    code += tab;
    code += "mla";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += ", ";
    code += op3.getName();
    code += newline;
    return code;
}

std::string gen_smmla(const Reg &target, const Reg &op1, const Reg &op2,
                  const Reg &op3) {
    std::string code;
    code += tab;
    code += "smmla";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += ", ";
    code += op3.getName();
    code += newline;
    return code;
}

std::string gen_mls(const Reg &target, const Reg &op1, const Reg &op2,
                const Reg &op3) {
    std::string code;
    code += tab;
    code += "mls";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += ", ";
    code += op3.getName();
    code += newline;
    return code;
}

std::string gen_smull(const Reg &target, const Reg &op1, const Reg &op2,
                  const Reg &op3) {
    std::string code;
    code += tab;
    code += "smull";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += ", ";
    code += op3.getName();
    code += newline;
    return code;
}

std::string gen_sdiv(const Reg &target, const Reg &op1, const Reg &op2) {
    std::string code;
    code += tab;
    code += "sdiv";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}

std::string gen_udiv(const Reg &target, const Reg &op1, const Reg &op2) {
    std::string code;
    code += tab;
    code += "udiv";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}


std::string gen_cmp(const Reg &lhs, const Value &rhs) {
    std::string code;
    code += tab;
    code += "cmp";
    code += " ";
    code += lhs.getName();
    code += ", ";
    code += rhs.getName();
    code += newline;
    return code;
}

std::string gen_b(const Label &target, const CmpOp &cond) {
    std::string code;
    code += tab;
    code += "b";
    code += condCode(cond);
    code += " ";
    code += target.getName();
    code += newline;
    return code;
}
/*
std::string instConst(std::string (*inst)(const Reg &target, const Reg &op1,
                                          const Value &op2),
                      const Reg &target, const Reg &op1, const Constant &op2) {
    std::string code;
    int val = op2.getValue();
    if (target == op1 && op2.getValue() == 0 && (inst == add || inst == sub)) {
        return code;
    } else if (0 <= val && val <= imm_8_max) {
        code += inst(target, op1, op2);
    } else {
        code += setValue(vinst_temp_reg, op2);
        code += inst(target, op1, vinst_temp_reg);
    }
    return code;
}

std::string instConst(std::string (*inst)(const Reg &op1, const Value &op2),
                      const Reg &op1, const Constant &op2) {
    std::string code;
    int val = op2.getValue();
    if (0 <= val && val <= imm_8_max) {
        code += inst(op1, op2);
    } else {
        code += setValue(vinst_temp_reg, op2);
        code += inst(op1, vinst_temp_reg);
    }
    return code;
}

std::string load(const Reg &target, const Addr &source) {
    std::string code;
    int offset = source.getOffset();
    if (offset > imm_12_max || offset < -imm_12_max) {
        code += InstGen::setValue(vinst_temp_reg, Constant(offset));
        code += InstGen::ldr(target, source.getReg(), vinst_temp_reg);
    } else 
        code += InstGen::ldr(target, source);
    return code;
}

std::string store(const Reg &source, const Addr &target) {
    assert(source != vinst_temp_reg);
    std::string code;
    int offset = target.getOffset();
    if (offset > imm_12_max || offset < -imm_12_max) {
        code += InstGen::setValue(vinst_temp_reg, Constant(offset));
        code += InstGen::str(source, target.getReg(), vinst_temp_reg);
    } else {
        code += InstGen::str(source, target);
    }
    return code;
}
*/
std::string gen_swi(const Constant &id) {
    std::string code;
    code += tab;
    code += "swi";
    code += " ";
    code += id.getName();
    code += newline;
    return code;
}

std::string gen_bic(const Reg &target, const Reg &op1, const Value &op2) {
    std::string code;
    code += tab;
    code += "bic";
    code += " ";
    code += target.getName();
    code += ", ";
    code += op1.getName();
    code += ", ";
    code += op2.getName();
    code += newline;
    return code;
}


/*
//TODO
std::string bic(const Reg &target, const Reg &v1, const Reg &v2,
                const Reg &v3) {
    std::string code;
    code += mov(vinst_temp_reg, Constant(1));
    code += tab + "bic" + " " + vinst_temp_reg.getName() + ", " +
                v1.getName() + ", " + vinst_temp_reg.getName() + ", " + "lsl" +
                " " + v2.getName() + newline;
    code += tab + "orr" + " " + target.getName() + ", " +
                vinst_temp_reg.getName() + ", " + v3.getName() + ", " + "lsl" +
                " " + v2.getName() + newline;
    return code;
}

std::tuple<int, int, int> choose_multiplier(int d, int N) {
    assert(d >= 1);
    int l = ceil(log2((double)d - 0.5));
    int sh_post = l;
    uint64_t m_l = (((uint64_t)1) << (N + l)) / d;
    uint64_t m_h = ((((uint64_t)1) << (N + l)) + (((uint64_t)1) << (l))) / d;
    while ((m_l / 2 < m_h / 2) && (sh_post > 1)) {
        m_l /= 2;
        m_h /= 2;
        sh_post--;
    }
    sh_post--;
    return std::make_tuple((int)(m_h), sh_post, l);
};

std::string divConst(const Reg &target, const Reg &source,
                     const Constant &divisor) {
    const int N = 32;
    std::string code;
    int d = divisor.getValue();
    assert(d != 0);
    int m, sh_post, l;
    std::tie(m, sh_post, l) = choose_multiplier(abs(d), N - 1);
    if (abs(d) == 1) {
        code += mov(target, source);
    } else if (abs(d) == (1 << l)) {
        // q = SRA(n + SRL(SRA(n, l - 1), N - l), l);
        code += asr(vinst_temp_reg, source, Constant(l - 1));
        code += lsr(vinst_temp_reg, vinst_temp_reg, Constant(N - l));
        code += add(vinst_temp_reg, vinst_temp_reg, source);
        code += asr(target, vinst_temp_reg, Constant(l));
    } else if (m >= 0) {
        // q = SRA(MULSH(m, n), sh_post) - XSIGN(n);
        code += setValue(vinst_temp_reg, Constant(m));
        code += smmul(vinst_temp_reg, vinst_temp_reg, source);
        code += asr(vinst_temp_reg, vinst_temp_reg, Constant(sh_post));
        code +=
            add(target, vinst_temp_reg,
                RegShift(source.getID(), 31, InstGen::RegShift::ShiftType::lsr));
    } else {
        // q = SRA(n + MULSH(m - 2^N , n), sh_post) - XSIGN(n);
        code += setValue(vinst_temp_reg, Constant(m));
        code += smmla(vinst_temp_reg, vinst_temp_reg, source, source);
        code += asr(vinst_temp_reg, vinst_temp_reg, Constant(sh_post));
        code +=
            add(target, vinst_temp_reg,
                RegShift(source.getID(), 31, InstGen::RegShift::ShiftType::lsr));
    }
    if (d < 0) {
        code += rsb(target, target, Constant(0));
    }
    return code;
}
*/
}; // namespace InstGen