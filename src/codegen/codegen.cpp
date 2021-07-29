#include "codegen/codegen.h"

#include <algorithm>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "codegen/instgen.h"
#include "IR/Module.h"


extern bool disable_div_optimization; // for debug
const std::string tab="    ";
const std::string newline="\n";
std::string codegen::generateModuleCode(std::map<Value *, int> reg_mapping) {
    std::string asm_code;
    this->reg_mapping = reg_mapping;
    asm_code += tab + ".arch armv" + std::to_string(arch_version) +
                "-a" + newline;
    asm_code += tab + ".text" + newline;
    for (auto &func : this->module->get_functions()) {
        if (func->get_basic_blocks().size()) {
            asm_code += codegen::globalVariable(func->get_name());
        }
    }
    asm_code += tab + ".arm" + newline;
    asm_code += tab + ".fpu neon" + newline;
    asm_code += codegen::generateGlobalTable();
    for (auto &func : this->module->get_functions()) {
        if (func->get_basic_blocks().size()) {
            asm_code += codegen::generateFunctionCode(func);
        }
    }
    asm_code += tab + ".data" + newline;
    asm_code += codegen::generateGlobalVarsCode();
    return asm_code;
}

std::string codegen::globalVariable(std::string name) {
    return tab + ".global" + " " + name + newline;
}

std::string codegen::generateFunctionCode(Function *func) {
    std::string asm_code;
    int counter = 0;
    codegen::allocateStackSpace(func);
    for (auto &bb : func->get_basic_blocks()) {
        if (bb->get_name().empty()) {
            bb->set_name(std::to_string(counter++));
        }
    }
    asm_code += func->get_name() + ":" + newline;
    asm_code +=
            codegen::comment("stack_size=" + std::to_string(this->stack_size));
    asm_code += generateFunctionEntryCode(func);
    for (auto &bb : func->get_basic_blocks()) {
        asm_code += getLabelName(bb) + ":" + newline;
        asm_code += generateBasicBlockCode(bb);
    }
    asm_code += generateFunctionPostCode(func);
    return asm_code;
}


std::string codegen::generateFunctionEntryCode(Function *func) {
    std::string asm_code;
    asm_code += codegen::getLabelName(func, 0) + ":" + newline;
    asm_code += codegen::comment("function preprocess");
    auto save_registers = codegen::getCalleeSaveRegisters(func);
    save_registers.push_back(InstGen::lr);
    std::sort(save_registers.begin(), save_registers.end());
    // large stack allocate
    if (func->get_name() == "main" && enlarge_stack) {
        const bool set_zero = false;
        asm_code += codegen::comment("enlarge stack");
        // do not change lr (r14)
        asm_code += InstGen::gen_push(save_registers);
        asm_code += InstGen::gen_set_value(InstGen::Reg(0),
                                      InstGen::Constant(enlarge_stack_size));
        asm_code += InstGen::gen_set_value(InstGen::Reg(1), InstGen::Constant(1));
        asm_code += InstGen::gen_bl(set_zero ? "calloc" : "malloc");
        asm_code += InstGen::gen_add(InstGen::Reg(op_reg_0),InstGen::Reg(0),InstGen::Constant(enlarge_stack_size));
        asm_code += InstGen::gen_pop(save_registers);
        asm_code += InstGen::gen_mov(InstGen::Reg(op_reg_2), InstGen::sp);
        asm_code += InstGen::gen_mov(InstGen::sp, InstGen::Reg(op_reg_0));
        asm_code += InstGen::gen_push({InstGen::Reg(op_reg_2)});
    }
    // save callee-save registers and lr
    asm_code += codegen::comment("save callee-save registers and lr");
    asm_code += InstGen::gen_push(save_registers);
    // allocate stack space and process function args
    asm_code += codegen::comment("allocate stack space");
    asm_code += InstGen::gen_sub(InstGen::sp,InstGen::sp,InstGen::Constant(this->stack_size));
    asm_code += codegen::comment("process function args");
    int cnt = 0;
    std::vector<Value *> source, target;
    Type dummy_type(Type::IntegerTy32ID,module);
    std::vector<Value> dummys(func->get_args().size(), Value(&dummy_type));
    for (auto &arg : func->get_args()) {
        bool extended = false;
        auto sizeof_val = arg->get_type()->get_size(extended);
        sizeof_val = ((sizeof_val + 3) / 4) * 4;
        auto dummy = &dummys.at(cnt);
        if (cnt >= 4) {
            int offset = stack_size + (save_registers.size() + cnt - 4) * 4;
            this->reg_mapping.erase(dummy);
            this->stack_mapping[dummy] = offset;
            source.push_back(dummy);
            target.push_back(arg);
        } else {
            this->reg_mapping[dummy] = cnt;
            this->stack_mapping.erase(dummy);
            source.push_back(dummy);
            target.push_back(arg);
        }
        cnt++;
    }
    asm_code += codegen::virtualRegMove(target, source);
    return asm_code;
}

std::string codegen::generateFunctionExitCode(Function *func) {
    std::string asm_code;
    asm_code += codegen::comment("function return");
    // reclaim stack space
    asm_code += codegen::comment("reclaim stack space");
    asm_code += InstGen::gen_add(InstGen::sp, InstGen::sp,
                                   InstGen::Constant(this->stack_size));
    // restore callee-save registers and pc
    asm_code += codegen::comment("restore callee-save registers and pc");
    auto save_registers = codegen::getCalleeSaveRegisters(func);
    save_registers.push_back(InstGen::pc);
    std::sort(save_registers.begin(), save_registers.end());
    asm_code += InstGen::gen_pop(save_registers);
    // enlarged stack reclaim
    if (func->get_name() == "main" && enlarge_stack) {
        asm_code += codegen::comment("enlarged stack reclaim");
        asm_code += InstGen::gen_pop({InstGen::Reg(op_reg_0)});
        asm_code += InstGen::gen_mov(InstGen::sp, InstGen::Reg(op_reg_0));
    }
    return asm_code;
}

std::string codegen::generateFunctionPostCode(Function *func) {
    std::string asm_code;
    asm_code += codegen::getLabelName(func, 1) + ":" + newline;
    asm_code += codegen::comment("function postcode");
    return asm_code;
}

std::string codegen::getLabelName(BasicBlock *bb) {
    return "." + bb->get_parent()->get_name() + "_" + bb->get_name();
}

std::string codegen::getLabelName(Function *func, int type) {
    const std::vector<std::string> name_list = {"pre", "post"};
    return "." + func->get_name() + "_" + name_list.at(type);
}

std::string codegen::generateBasicBlockCode(BasicBlock *bb) {
    std::string asm_code;
    for (auto &instr : bb->get_instructions()) {
        asm_code += codegen::generateInstructionCode(instr);
    }
    return asm_code;
}

std::string codegen::generateInstructionCode(Instruction *instr) {
    std::string asm_code;
    auto &oprands = instr->get_operands();
    if (instr->get_instr_type() == Instruction::ret) {
        if (!oprands.empty()) {
            asm_code += codegen::assignSpecificReg(oprands.at(0), 0); // ABI
        }
        asm_code += codegen::generateFunctionExitCode(instr->get_function());
    }
    else if (instr->get_instr_type() == Instruction::alloca) {
        if (this->reg_mapping.count(instr)) {
            auto offset = stack_mapping.at(instr);
            int target = this->reg_mapping.at(instr);
            asm_code += InstGen::gen_add(InstGen::Reg(target),InstGen::sp, InstGen::Constant(offset));
        }
        bool need_init = static_cast<AllocaInst *>(instr)->get_init();
        int init_val = 0;
        if (need_init) {
            const int init_threshold = 0;
            int sz = instr->get_type()->get_size();
            if (sz > init_threshold * 4) {
                Type integer_type(Type::IntegerTy32ID, module);
                std::vector<Value *> args = {
                        instr, ConstantInt::get(init_val, module),
                        ConstantInt::get(sz, module)};
                asm_code += codegen::generateFunctionCall(instr, "memset", args, -1);
            }
            else {
                asm_code += InstGen::gen_mov(InstGen::Reg(op_reg_0), InstGen::Constant(0));
                asm_code += codegen::assignSpecificReg(instr, op_reg_1);
                for (int i = 0; i < sz; i += 4) {
                    asm_code += tab + "str" + " " + InstGen::Reg(op_reg_0).getName() + ", " +
                            "[" + InstGen::Reg(op_reg_1).getName() + ", " + InstGen::Constant(4).getName() + "]" + "!" + newline; // NOLINT(performance-inefficient-string-concatenation)
                }
            }
        }
    }
    else if (instr->get_instr_type() == Instruction::load) {
        int alu_op0 = this->reg_mapping.count(oprands.at(0))
                      ? this->reg_mapping.at(oprands.at(0))
                      : op_reg_0;
        int alu_ret = this->reg_mapping.count(instr)
                      ? this->reg_mapping.at(instr)
                      : op_reg_1;
        asm_code += codegen::assignSpecificReg(oprands.at(0), alu_op0);
        if (oprands.size() >= 2) {
            ConstantInt *op1_const = dynamic_cast<ConstantInt *>(oprands.at(1));
            int shift = 0;
            if (oprands.size() >= 3) {
                ConstantInt *op2_const = dynamic_cast<ConstantInt *>(oprands.at(2));
                shift = op2_const->get_value();
            }
            if (op1_const) {
                asm_code +=
                        InstGen::load(InstGen::Reg(alu_ret),
                                      InstGen::Addr(InstGen::Reg(alu_op0),
                                                    op1_const->get_value() << shift));
            }
            else {
                int alu_op1 = this->reg_mapping.count(oprands.at(1))
                              ? this->reg_mapping.at(oprands.at(1))
                              : op_reg_1;
                asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                asm_code +=
                        InstGen::gen_ldr(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),
                                     InstGen::Reg(alu_op1), InstGen::Constant(shift));
            }
        }
        else {
            asm_code += InstGen::load(InstGen::Reg(alu_ret),
                                      InstGen::Addr(InstGen::Reg(alu_op0), 0));
        }
        asm_code += codegen::getSpecificReg(instr, alu_ret);
    }
    else if (instr->get_instr_type() == Instruction::store) {
        int alu_op0 = this->reg_mapping.count(oprands.at(0))
                      ? this->reg_mapping.at(oprands.at(0))
                      : op_reg_0;
        int alu_op1 = this->reg_mapping.count(oprands.at(1))
                      ? this->reg_mapping.at(oprands.at(1))
                      : op_reg_1;
        asm_code += codegen::assignSpecificReg(oprands.at(0), alu_op0);
        asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
        if (oprands.size() >= 3) {
            ConstantInt *op2_const = dynamic_cast<ConstantInt *>(oprands.at(2));
            int shift = 0;
            if (oprands.size() >= 4) {
                ConstantInt *op3_const = dynamic_cast<ConstantInt *>(oprands.at(3));
                shift = op3_const->get_value();
            }
            if (op2_const) {
                asm_code +=
                        InstGen::store(InstGen::Reg(alu_op0),
                                       InstGen::Addr(InstGen::Reg(alu_op1),
                                                     op2_const->get_value() << shift));
            }
            else {
                int alu_op2 = this->reg_mapping.count(oprands.at(2))
                              ? this->reg_mapping.at(oprands.at(2))
                              : op_reg_2;
                asm_code += codegen::assignSpecificReg(oprands.at(2), alu_op2);
                asm_code +=
                        InstGen::gen_str(InstGen::Reg(alu_op0), InstGen::Reg(alu_op1),
                                     InstGen::Reg(alu_op2), InstGen::Constant(shift));
            }
        }
        else {
            asm_code += InstGen::gen_str(InstGen::Reg(alu_op0),
                                     InstGen::Addr(InstGen::Reg(alu_op1), 0));
        }
    }
    else if (instr->get_instr_type() == Instruction::call) {
        std::string func_name = oprands.at(0)->get_name();
        std::vector<Value *> args(oprands.begin() + 1, oprands.end());
        asm_code += generateFunctionCall(instr, func_name, args);
    }
    else if (
               instr->get_instr_type() == Instruction::add ||
               instr->get_instr_type() == Instruction::sub ||
               instr->get_instr_type() == Instruction::mul ||
               instr->get_instr_type() == Instruction::cmp ||
               instr->get_instr_type() == Instruction::zext ||
               (instr->get_instr_type() == Instruction::getelementptr &&
                instr->get_operands().size() == 2) ||
               instr->get_instr_type() == Instruction::sdiv /*||
               instr->get_instr_type() == Instruction::rem*/) {
        ConstantInt *op1_const =
                (oprands.size() >= 2) ? (dynamic_cast<ConstantInt *>(oprands.at(1))) : nullptr;
        int alu_op0 = this->reg_mapping.count(oprands.at(0))
                      ? this->reg_mapping.at(oprands.at(0))
                      : op_reg_0;
        int alu_op1 = -1;
        if (oprands.size() >= 2) {
            alu_op1 = this->reg_mapping.count(oprands.at(1))
                      ? this->reg_mapping.at(oprands.at(1))
                      : op_reg_1;
        }
        int alu_ret = this->reg_mapping.count(instr)
                      ? this->reg_mapping.at(instr)
                      : op_reg_0;
        // flexible operand2
        int shift = 0;
        if (oprands.size() >= 3 && (instr->get_instr_type() == Instruction::add ||
                                instr->get_instr_type() == Instruction::sub ||
                                instr->get_instr_type() == Instruction::cmp)) {
            ConstantInt *op2_const = dynamic_cast<ConstantInt *>(oprands.at(2));
            shift = op2_const->get_value();
        }
        // must not change value of alu_op0 alu_op1
        asm_code += codegen::assignSpecificReg(oprands.at(0), alu_op0);
        if (instr->get_instr_type() == Instruction::cmp) {
            InstGen::CmpOp asmCmpOp = codegen::cmpConvert(static_cast<CmpInst *>(instr)->get_cmp_op(), false);
            asm_code += InstGen::gen_mov(InstGen::Reg(alu_ret), InstGen::Constant(0));
            asm_code += InstGen::gen_mov(InstGen::Reg(alu_ret), InstGen::Constant(1), asmCmpOp);
        }
        else {
            bool flag = false;
            auto temp = dynamic_cast<ConstantInt *> (instr->get_operand(1));
            switch (instr->get_instr_type()) {
                case Instruction::add:
                    asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                    asm_code += InstGen::gen_add(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),
                                                     InstGen::RegShift(alu_op1, shift));
                    break;
                case Instruction::sub:
                    asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                    asm_code += InstGen::gen_sub(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),
                                                     InstGen::RegShift(alu_op1, shift));
                    break;
                case Instruction::mul:
                    if (temp != nullptr){
                        for (int i=1;i<=65536;i*=2){
                            if (temp->get_value()&i){
                                asm_code+=InstGen::gen_lsl(InstGen::vinst_temp_reg, InstGen::Reg(alu_op0), InstGen::Constant(i));
                                asm_code+=InstGen::gen_add(InstGen::Reg(alu_ret),   InstGen::Reg(alu_op0), InstGen::vinst_temp_reg);
                            }
                        }
                    }
                    else{
                        asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                        asm_code += InstGen::gen_mul(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),InstGen::Reg(alu_op1));
                    }
                    break;
                case Instruction::sdiv:
                    asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                    asm_code += InstGen::gen_sdiv(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),InstGen::Reg(alu_op1));
                    break;

                case Instruction::zext:
                    asm_code += InstGen::gen_mov(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0));
                    break;
                case Instruction::getelementptr:
                    asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
                    asm_code += InstGen::gen_set_value(
                            InstGen::Reg(op_reg_2),
                            InstGen::Constant(instr->get_type()->get_pointer_element_type()->get_size()));
                    asm_code += InstGen::gen_mul(InstGen::Reg(op_reg_2), InstGen::Reg(op_reg_2),InstGen::Reg(alu_op1));
                    asm_code += InstGen::gen_add(InstGen::Reg(alu_ret), InstGen::Reg(alu_op0),InstGen::Reg(op_reg_2));
                    break;
                default:
                    std::cerr << "???" << std::endl;
                    abort();
                    break;
            }
        }
        asm_code += codegen::getSpecificReg(instr, alu_ret);
    }
    else if (instr->get_instr_type() == Instruction::br) {
        auto inst_br = dynamic_cast<BranchInst *>(instr);
        const bool is_cond = inst_br->is_cond_br();
        const bool is_cmp = inst_br->is_cmp();
        const int i_start = is_cmp ? 2 : (is_cond ? 1 : 0);
        const int i_end = is_cmp ? 3 : (is_cond ? 2 : 0);
        bool swap_branch = false;
        std::map<int, bool> need_resolve;
        std::map<int, bool> need_jump;
        BasicBlock *bb_cur = instr->get_parent();
        // need phi resolve?
        {
            int branch_cnt = 0;
            BasicBlock *succ_bb = nullptr;
            bool flag = false;
            for (auto &for_bb : instr->get_function()->get_basic_blocks()) {
                if (flag) {
                    succ_bb = for_bb;
                    break;
                }
                if (for_bb == bb_cur) {
                    flag = true;
                }
            }
            for (int i = i_start; i <= i_end; i++) {
                std::vector<std::pair<Value *, Value *>> phis;
                auto bb_next = static_cast<BasicBlock *>(oprands.at(i));
                for (auto &inst_phi : bb_next->get_instructions()) {
                    if (inst_phi->is_phi()) {
                        Value *pre_op = nullptr;
                        for (auto &op_phi : inst_phi->get_operands()) {
                            if (dynamic_cast<BasicBlock *>(op_phi) &&
                                dynamic_cast<BasicBlock *>(op_phi)->get_terminator() == instr) {
                                phis.push_back({inst_phi, pre_op});
                            }
                            pre_op = op_phi;
                        }
                    }
                }
                std::vector<Value *> target, source;
                for (auto &p : phis) {
                    target.push_back(p.first);
                    source.push_back(p.second);
                }
                branch_cnt++;
                need_resolve[branch_cnt] =
                        (!codegen::virtualRegMove(target, source).empty());
                need_jump[branch_cnt] = (succ_bb != bb_next);
            }
        }
        // swap?
        {
            if ((is_cmp || is_cond) &&
                ((!need_resolve[1] && !need_resolve[2] && !need_jump[2]) ||
                 (!need_resolve[1] && need_resolve[2]) ||
                 (need_resolve[1] && need_resolve[2] && !need_jump[1]))) {
                std::swap(oprands[i_start], oprands[i_end]);
                std::swap(need_resolve[1], need_resolve[2]);
                std::swap(need_jump[1], need_jump[2]);
                swap_branch = true;
            }
        }
        // translate Br instr
        std::map<int, bool> elim;
        if (is_cmp) {
            ConstantInt *op1_const = (oprands.size() >= 2)
                                     ? (dynamic_cast<ConstantInt *>(oprands.at(1)))
                                     : nullptr;
            int alu_op0 = this->reg_mapping.count(oprands.at(0))
                          ? this->reg_mapping.at(oprands.at(0))
                          : op_reg_0;
            int alu_op1 = this->reg_mapping.count(oprands.at(1))
                          ? this->reg_mapping.at(oprands.at(1))
                          : op_reg_1;
            asm_code += codegen::assignSpecificReg(oprands.at(0), alu_op0);
            InstGen::CmpOp asmCmpOp = InstGen::GT;
            asm_code += codegen::assignSpecificReg(oprands.at(1), alu_op1);
            asm_code += InstGen::gen_cmp(InstGen::Reg(alu_op0), InstGen::Reg(alu_op1));
            if (need_resolve[2]) {
                asm_code +=
                        InstGen::gen_b(InstGen::Label(codegen::getLabelName(bb_cur) +
                                                  "_branch_" + std::to_string(2),
                                                  0),asmCmpOp);
            } else {
                auto bb_next = static_cast<BasicBlock *>(oprands.at(i_end));
                asm_code += InstGen::gen_b(
                        InstGen::Label(codegen::getLabelName(bb_next), 0), asmCmpOp);
                elim[2] = true;
            }
        }
        else if (is_cond) {
            InstGen::CmpOp asmCmpOp=InstGen::GT;
            int alu_op0 = this->reg_mapping.count(oprands.at(0))
                          ? this->reg_mapping.at(oprands.at(0))
                          : op_reg_0;
            asm_code += codegen::assignSpecificReg(oprands.at(0), alu_op0);
            asm_code += InstGen::gen_cmp(InstGen::Reg(alu_op0), InstGen::Constant(0));
            if (need_resolve[2]) {
                asm_code +=
                        InstGen::gen_b(InstGen::Label(codegen::getLabelName(bb_cur) +
                                                  "_branch_" + std::to_string(2),
                                                  0),
                                   asmCmpOp);
            }
            else {
                auto bb_next = static_cast<BasicBlock *>(oprands.at(i_end));
                asm_code += InstGen::gen_b(
                        InstGen::Label(codegen::getLabelName(bb_next), 0), asmCmpOp);
                elim[2] = true;
            }
        }
        // for each branch
        {
            int branch_cnt = 0;
            for (int i = i_start; i <= i_end; i++) {
                std::vector<std::pair<Value *, Value *>> phis;
                auto bb_next = static_cast<BasicBlock *>(oprands.at(i));
                for (auto &inst_phi : bb_next->get_instructions()) {
                    if (inst_phi->is_phi()) {
                        Value *pre_op = nullptr;
                        for (auto &op_phi : inst_phi->get_operands()) {
                            if (dynamic_cast<BasicBlock *>(op_phi) &&
                                dynamic_cast<BasicBlock *>(op_phi)->get_terminator() == instr) {
                                phis.push_back({inst_phi, pre_op});
                            }
                            pre_op = op_phi;
                        }
                    }
                }
                asm_code += codegen::getLabelName(bb_cur) + "_branch_" +
                            std::to_string(++branch_cnt) + ":" + newline;
                std::vector<Value *> target, source;
                for (auto &p : phis) {
                    asm_code +=
                            codegen::comment("%" + p.first->get_name() + " <= " +
                                             (dynamic_cast<Constant *>(p.second) ? "" : "%") +
                                             p.second->get_name());
                    target.push_back(p.first);
                    source.push_back(p.second);
                }
                if (need_resolve[branch_cnt]) {
                    codegen::comment("PHI resolve code");
                    asm_code += codegen::virtualRegMove(target, source);
                }
                if ((branch_cnt == 1 && !need_jump[1] && !need_resolve[2] &&
                     (!need_jump[2] || elim[2])) ||
                    (branch_cnt == 2 && (!need_jump[2] || elim[2]))) {
                    asm_code += codegen::comment("branch instruction eliminated");
                } else {
                    asm_code +=
                            InstGen::gen_b(InstGen::Label(codegen::getLabelName(bb_next), 0),
                                       InstGen::CmpOp::NOP);
                }
            }
        }
    }
    else if (instr->get_instr_type() == Instruction::phi) {
    }
    else {
        std::cerr << "Cannot translate this function:" << std::endl;
        instr->get_function()->print();
        std::cerr << std::endl;
        std::cerr << "Cannot translate this instruction:" << std::endl;
        instr->print();
        std::cerr << std::endl;
        abort();
    }
    return asm_code;
}

std::string codegen::generateFunctionCall(Instruction *instr,
                                          std::string func_name,
                                          std::vector<Value *> oprands,
                                          int return_reg, int sp_ofs) {
    std::string asm_code;
    assert(-1 <= return_reg && return_reg <= 3);
    auto in_func = instr->get_function();
    auto save_registers = codegen::getCallerSaveRegisters(in_func);
    if (active_vars.count(instr)) {
        // std::cerr << "Using ABI optimization" << std::endl;
        std::set<int> regs_set;
        for (auto &v : this->active_vars.at(instr)) {
            if (this->reg_mapping.count(v)) {
                regs_set.insert(this->reg_mapping.at(v));
            }
        }
        save_registers.clear();
        for (auto &r : regs_set) {
            save_registers.push_back(InstGen::Reg(r));
        }
    }
    bool has_return_value = (return_reg >= 0) && (instr->get_type()->get_size() > 0);
    // do not save returned register
    if (has_return_value && this->reg_mapping.count(instr)) {
        auto returned_reg = InstGen::Reg(this->reg_mapping.at(instr));
        decltype(save_registers) new_save_registers;
        for (auto &reg : save_registers) {
            if (reg.getID() != returned_reg.getID()) {
                new_save_registers.push_back(reg);
            }
        }
        save_registers = new_save_registers;
    }
    std::sort(save_registers.begin(), save_registers.end());
    if (!save_registers.empty()) {
        asm_code += InstGen::gen_push(save_registers);
    }
    int saved_regs_size = save_registers.size() * 4;
    int total_args_size = 0;
    // prepare args
    {
        std::vector<Value *> source, target;
        Type dummy_type(Type::IntegerTy32ID,module);
        std::vector<Value> dummys(oprands.size(), Value(&dummy_type));
        // args 0 1 2 3
        {
            for (int i = 0; i < std::min(oprands.size(), (size_t)4); i++) {
                auto dummy = &dummys.at(i);
                this->reg_mapping[dummy] = i;
                this->stack_mapping.erase(dummy);
                target.push_back(dummy);
                source.push_back(oprands.at(i));
            }
        }
        // args 4+
        {
            for (int i = oprands.size() - 1; i >= 4; i--) {
                total_args_size += 4;
                auto dummy = &dummys.at(i);
                this->reg_mapping.erase(dummy);
                this->stack_mapping[dummy] = -(total_args_size + saved_regs_size);
                target.push_back(dummy);
                source.push_back(oprands.at(i));
            }
        }
        asm_code +=
                codegen::virtualRegMove(target, source, saved_regs_size + sp_ofs);
    }
    asm_code += InstGen::gen_sub(InstGen::sp, InstGen::sp,
                                   InstGen::Constant(total_args_size));
    asm_code += InstGen::gen_bl(func_name);
    asm_code += InstGen::gen_add(InstGen::sp, InstGen::sp,
                                   InstGen::Constant(total_args_size));
    if (has_return_value) {
        asm_code +=
                codegen::getSpecificReg(instr, return_reg, saved_regs_size + sp_ofs);
    }
    if (!save_registers.empty()) {
        asm_code += InstGen::gen_pop(save_registers);
    }
    return asm_code;
}

std::vector<InstGen::Reg> codegen::getAllRegisters(Function *func) {
    std::set<InstGen::Reg> registers;
    for (auto &arg : func->get_args()) {
        if (this->reg_mapping.count(arg) &&
            this->reg_mapping.at(arg) <= InstGen::max_reg_id) {
            registers.insert(InstGen::Reg(this->reg_mapping.at(arg)));
        }
    }
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &instr : bb->get_instructions()) {
            if (this->reg_mapping.count(instr) &&
                this->reg_mapping.at(instr) <= InstGen::max_reg_id) {
                registers.insert(InstGen::Reg(this->reg_mapping.at(instr)));
            }
        }
    }
    for (auto &reg : temp_regs) { // used as temp regs
        registers.insert(reg);
    }
    return std::vector<InstGen::Reg>(registers.begin(), registers.end());
}

std::vector<InstGen::Reg> codegen::getCalleeSaveRegisters(Function *func) {
    std::set<InstGen::Reg> registers;
    for (auto &reg : codegen::getAllRegisters(func)) {
        if (callee_save_regs.count(reg)) {
            registers.insert(reg);
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(), registers.end());
}

std::vector<InstGen::Reg> codegen::getCallerSaveRegisters(Function *func) {
    std::set<InstGen::Reg> registers;
    for (auto &reg : codegen::getAllRegisters(func)) {
        if (caller_save_regs.count(reg) && !temp_regs.count(reg)) {
            registers.insert(reg);
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(), registers.end());
}

void codegen::allocateStackSpace(Function *func) {
    this->stack_size = 0;
    this->stack_mapping.clear();
    for (auto &arg : func->get_args()) {
        bool extended = false;
        auto sizeof_val = arg->get_type()->get_size(extended);
        sizeof_val = ((sizeof_val + 3) / 4) * 4;
        assert(sizeof_val == 4);
        if (!reg_mapping.count(arg)) {
            stack_mapping[arg] = this->stack_size;
            this->stack_size += sizeof_val;
        }
    }
    // non alloca space and non alloca pointer
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &instr : bb->get_instructions()) {
            if (this->reg_mapping.count(instr)) {
                int map_reg_id = this->reg_mapping.at(instr);
                if (map_reg_id > InstGen::max_reg_id) {
                    reg_mapping.erase(instr);
                }
                if (!allocate_regs.count(InstGen::Reg(map_reg_id))) {
                    std::cerr << "Reg " << map_reg_id << " should not be allocated"
                              << std::endl;
                    abort();
                }
            }
            if (this->reg_mapping.count(instr)) {
                continue;
            }
            if (this->stack_mapping.count(instr)) {
                continue;
            }
            if (instr->is_alloca()) {
                continue;
            }
            bool extended = false;
            auto sizeof_val = instr->get_type()->get_size(extended);
            sizeof_val = ((sizeof_val + 3) / 4) * 4;
            if (sizeof_val > 0) {
                this->stack_mapping[instr] = this->stack_size;
                this->stack_size += sizeof_val;
            }
        }
    }
    // alloca space
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &instr : bb->get_instructions()) {
            if (!instr->is_alloca()) {
                continue;
            }
            bool extended = true;
            this->allocated.insert(instr);
            auto sizeof_val = instr->get_type()->get_size(extended);
            sizeof_val = ((sizeof_val + 3) / 4) * 4;
            if (sizeof_val > 0) {
                this->stack_mapping[instr] = this->stack_size;
                this->stack_size += sizeof_val;
            }
        }
    }
}

bool codegen::isSameMapping(Value *a, Value *b) {
    if (this->reg_mapping.count(a) && this->reg_mapping.count(b)) {
        return this->reg_mapping.at(a) == this->reg_mapping.at(b);
    }
    if (this->stack_mapping.count(a) && this->stack_mapping.count(b)) {
        return this->stack_mapping.at(a) == this->stack_mapping.at(b);
    }
    return false;
}

std::string codegen::virtualRegMove(std::vector<Value *> target,
                                    std::vector<Value *> source, int sp_ofs) {
    std::string asm_code;
    assert(target.size() == source.size());
    int sz = target.size();
    std::list<std::pair<Value *, Value *>> L;
    for (int i = 0; i < sz; i++) {
        L.push_back({target.at(i), source.at(i)});
    }
    for (auto it = L.begin(); it != L.end(); it++) {
        for (auto it2 = L.begin(); it2 != L.end(); it2++) {
            if (it2 != it && codegen::isSameMapping(it2->first, it->first)) {
                std::cerr << "virtualRegMove race condition" << std::endl;
                abort();
            }
        }
    }
    Value *tg_val = nullptr;
    while (!L.empty()) {
        bool flag = true;
        for (auto it = L.begin(); it != L.end(); it++) {
            bool ok = true;
            for (auto it2 = L.begin(); it2 != L.end(); it2++) {
                if (it2 != it && codegen::isSameMapping(it2->second, it->first)) {
                    ok = false;
                }
            }
            if (ok) {
                asm_code += codegen::virtualRegMove(it->first, it->second, sp_ofs);
                L.erase(it);
                flag = false;
                break;
            }
        }
        if (flag) {
            if (tg_val != nullptr) {
                asm_code += codegen::getSpecificReg(tg_val, op_reg_0, sp_ofs);
            }
            auto it = L.begin();
            asm_code += codegen::assignSpecificReg(it->second, op_reg_0, sp_ofs);
            tg_val = it->first;
            L.erase(it);
        }
    }
    if (tg_val != nullptr) {
        asm_code += codegen::getSpecificReg(tg_val, op_reg_0, sp_ofs);
    }
    return asm_code;
}

std::string codegen::virtualRegMove(Value *target, Value *source, int sp_ofs) {
    std::string asm_code;
    if (codegen::isSameMapping(target, source)) {
        return asm_code;
    }
    int alu_op0 = this->reg_mapping.count(target)
                  ? this->reg_mapping.at(target)
                  : op_reg_0;
    asm_code += codegen::assignSpecificReg(source, alu_op0, sp_ofs);
    asm_code += codegen::getSpecificReg(target, alu_op0, sp_ofs);
    return asm_code;
}

std::string codegen::assignSpecificReg(Value *val, int target, int sp_ofs) {
    std::string asm_code;
    auto val_const = dynamic_cast<ConstantInt *>(val);
    auto val_global = dynamic_cast<GlobalVariable *>(val);
    if (val_const) {
        int imm = val_const->get_value();
        asm_code += InstGen::gen_set_value(InstGen::Reg(target), InstGen::Constant(imm));
    }
    else if (val_global) { // need optimization
        asm_code += InstGen::gen_adrl(
                InstGen::Reg(op_reg_2),
                InstGen::Label(global_vars_label, codegen::getGlobalAddress(val_global) * 4));
        asm_code += InstGen::gen_ldr(InstGen::Reg(target),
                                 InstGen::Addr(InstGen::Reg(op_reg_2), 0));
    }
    else if (reg_mapping.count(val) &&
               reg_mapping.at(val) <= InstGen::max_reg_id) {
        auto source = reg_mapping.at(val);
        asm_code += InstGen::gen_mov(InstGen::Reg(target), InstGen::Reg(source));
    }
    else if (allocated.count(val)) {
        auto offset = stack_mapping.at(val) + sp_ofs;
        asm_code += InstGen::gen_add(InstGen::Reg(target),
                                       InstGen::sp, InstGen::Constant(offset));
    }
    else if (stack_mapping.count(val)) {
        auto offset = stack_mapping.at(val) + sp_ofs;
        asm_code += InstGen::load(InstGen::Reg(target),
                                      InstGen::Addr(InstGen::sp, offset));
    }
    else {
        std::cerr << "Function assignSpecificReg exception!" << std::endl;
        abort();
    }
    return asm_code;
}

std::string codegen::getSpecificReg(Value *val, int source, int sp_ofs) {
    std::string asm_code;
    if (reg_mapping.count(val) &&
        reg_mapping.at(val) <= InstGen::max_reg_id) {
        auto target = reg_mapping.at(val);
        asm_code += InstGen::gen_mov(InstGen::Reg(target), InstGen::Reg(source));
    } else if (stack_mapping.count(val)) {
        auto offset = stack_mapping.at(val) + sp_ofs;
        asm_code += InstGen::store(InstGen::Reg(source),InstGen::Addr(InstGen::sp, offset));
    } 
    else {
        std::cerr << "Function getFromSpecificReg exception!" << std::endl;
        abort();
    }
    return asm_code;
}

std::string codegen::generateGlobalTable() {
    std::string asm_code;
    this->global_table.clear();
    for (auto &global_var : this->module->get_global_variable()) {
        int count = this->global_table.size();
        if (!global_table.count(global_var)) {
            this->global_table[global_var] = count;
        }
    }
    std::vector<Value *> vecGOT;
    vecGOT.resize(this->global_table.size());
    for (auto &i : global_table) {
        vecGOT[i.second] = i.first;
    }
    for (auto &i : vecGOT) {
        asm_code += tab + ".global" + " " + i->get_name() + newline;
    }
    asm_code += global_vars_label + ":" + newline;
    for (auto &i : vecGOT) {
        asm_code += tab + ".long" + " " + i->get_name() + newline;
    }
    return asm_code;
}

int codegen::getGlobalAddress(Value *val) { return this->global_table.at(val); }

std::string codegen::generateGlobalVarsCode() {
    std::string asm_code;
    for (auto &global_var : this->module->get_global_variable()) {
        asm_code += global_var->get_name() + ":" + newline;
        if (! Type::is_eq_type(global_var->get_type()->get_pointer_element_type(),
                global_var->get_operands().at(0)->get_type())) {
            asm_code += tab + ".zero" + " " +
                        std::to_string(global_var->get_type()->get_size()) +
                        newline;
        } else {
            asm_code += codegen::generateInitializerCode(
                    static_cast<Constant *>(global_var->get_operands().at(0)));
        }
    }
    return asm_code;
}

std::string codegen::generateInitializerCode(Constant *init) {
    std::string asm_code;
    auto array_init = dynamic_cast<ConstantArray *>(init);
    if (array_init) {
        auto length =
                static_cast<ArrayType *>(array_init->get_type())->get_num_of_elements();
        for (int i = 0; i < length; i++) {
            asm_code +=
                    codegen::generateInitializerCode(array_init->get_element_value(i));
        }
    } else {
        auto val = codegen::constIntVal(init);
        if (!val.second) {
            std::cerr << "Function generateInitializerCode exception!" << std::endl;
            abort();
        }
        asm_code += tab + ".long" + " " + std::to_string(val.first) +
                    newline;
    }
    return asm_code;
}

std::pair<int, bool> codegen::constIntVal(Value *val) { // disabled
    auto const_val = dynamic_cast<ConstantInt *>(val);
    auto inst_val = dynamic_cast<Instruction *>(val);
    if (const_val) {
        return std::make_pair(const_val->get_value(), true);
    }
    else if (inst_val && false) {
        auto op_list = inst_val->get_operands();
        if (dynamic_cast<BinaryInst *>(val)) {
            auto val_0 = codegen::constIntVal(op_list.at(0));
            auto val_1 = codegen::constIntVal(op_list.at(1));
            if (val_0.second && val_1.second) {
                int ret = 0;
                bool flag = true;
                switch (inst_val->get_instr_type()) {
                    case Instruction::add:
                        ret = val_0.first + val_1.first;
                        break;
                    case Instruction::sub:
                        ret = val_0.first - val_1.first;
                        break;
                    default:
                        flag = false;
                        break;
                }
                return std::make_pair(ret, flag);
            }
            else {
                return std::make_pair(0, false);
            }
        }
    }
    std::cerr << "Function getConstIntVal exception!" << std::endl;
    abort();
}

std::string codegen::comment(std::string s) {
    std::string asm_code;
    asm_code += tab + "@ " + s + newline;
    return asm_code;
}

InstGen::CmpOp codegen::cmpConvert(CmpInst::CmpOp myCmpOp, bool reverse) {
    InstGen::CmpOp asmCmpOp;
    if (!reverse) {
        switch (myCmpOp) {
            case CmpInst::CmpOp::EQ:
                asmCmpOp = InstGen::CmpOp::EQ;
                break;
            case CmpInst::CmpOp::NE:
                asmCmpOp = InstGen::CmpOp::NE;
                break;
            case CmpInst::CmpOp::GT:
                asmCmpOp = InstGen::CmpOp::GT;
                break;
            case CmpInst::CmpOp::GE:
                asmCmpOp = InstGen::CmpOp::GE;
                break;
            case CmpInst::CmpOp::LT:
                asmCmpOp = InstGen::CmpOp::LT;
                break;
            case CmpInst::CmpOp::LE:
                asmCmpOp = InstGen::CmpOp::LE;
                break;
            default:
                std::cerr << "CmpOp type not valid" << std::endl;
                abort();
        }
    } else {
        switch (myCmpOp) {
            case CmpInst::CmpOp::EQ:
                asmCmpOp = InstGen::CmpOp::NE;
                break;
            case CmpInst::CmpOp::NE:
                asmCmpOp = InstGen::CmpOp::EQ;
                break;
            case CmpInst::CmpOp::GT:
                asmCmpOp = InstGen::CmpOp::LE;
                break;
            case CmpInst::CmpOp::GE:
                asmCmpOp = InstGen::CmpOp::LT;
                break;
            case CmpInst::CmpOp::LT:
                asmCmpOp = InstGen::CmpOp::GE;
                break;
            case CmpInst::CmpOp::LE:
                asmCmpOp = InstGen::CmpOp::GT;
                break;
            default:
                std::cerr << "CmpOp type not valid" << std::endl;
                abort();
        }
    }
    return asmCmpOp;
}
