//
// Created by mskhana on 2021/7/20.
//

#include <algorithm>
#include "include/codegen/codegen.h"
#include "include/codegen/instgen.h"
//#include "include/codegen/regAlloc.h"
const std::string tab="    ";
const std::string newline="\n";
std::string codegen::generateModuleCode(std::map<Value *, int> reg_mapping, bool auto_alloc) {
    std::string asm_code;
    asm_code+=tab+".arch armv"+std::to_string(arch_version)+"-a";
    asm_code+=tab+".file \""+"\"";
    asm_code+=tab+".text";
    for (auto &func:this->module->get_functions()){
        if (func->get_basic_blocks().size())
            asm_code+=codegen::globalVariable(func->get_name());
    }
    asm_code+=tab+".arm";
    asm_code+=tab+".arm";
    asm_code+=generateGlobalTable();
    for (auto &func:this->module->get_functions()){
        if (func->get_basic_blocks().size()){
            asm_code+=codegen::generateFunctionCode(func);
        }
    }
    asm_code+=tab+".data";
    asm_code+=generateGlobalVarsCode();
    return asm_code;
}

std::string codegen::generateFunctionCode(Function *func) {
    std::string asm_code;
    asm_code+=codegen::allocate_stack(func);
    //reg_mapping=regAlloc(func);
    int counter=0;
    for (auto &bb: func->get_basic_blocks()){
        if (bb->get_name().empty()){
            bb->set_name(std::to_string (counter++));
        }
    }
    asm_code+=func->get_name()+":";
    asm_code+=codegen::comment("stack_size="+std::to_string(this->stack_size));
    asm_code+=codegen::generateFunctionEntryCode(func);

    for (auto &bb: func->get_basic_blocks()){
        asm_code+=codegen::generateBasicBlockCode(bb);
    }

    asm_code+=codegen::generateFunctionExitCode(func);
    return asm_code;
}

std::string codegen::allocate_stack(Function *func){
    std::string asm_code;
    this->stack_size=0;
    this->stack_mapping.clear();
    //allocate function args
    for (auto &arg: func->get_args()){
        if (!reg_mapping.count(arg)){
            stack_mapping[arg]=this->stack_size;
            this->stack_size+=4;
        }
    }
    for (auto &bb: func->get_basic_blocks()){
        for (auto &inst: bb->get_instructions()){
            if (inst->is_alloca()){
                this->allocated.insert(inst);
                auto sizeof_val=inst->get_type()->get_size();
                sizeof_val=((sizeof_val+3)/4)*4;
                if (sizeof_val){
                    this->stack_mapping[inst]=this->stack_size;
                    this->stack_size+=sizeof_val;
                }
            }
        }
    }
}

std::string codegen::generateFunctionEntryCode(Function *func) {
    std::string asm_code;
    asm_code.clear();
    asm_code+=codegen::getFunctionLabelName(func,0)+":";
    asm_code+=codegen::comment("function initialize");
    auto save_register=codegen::getCalleeSaveRegisters(func);
    save_register.push_back(InstGen::Reg(14)); //lr reg
    std::sort(save_register.begin(),save_register.end());
    //large stack allocate
    asm_code+=InstGen::gen_push(save_register);
    // save callee register and lr
    asm_code+=InstGen::gen_sub(InstGen::Reg(13),InstGen::Reg(13),InstGen::Constant(this->stack_size));
    // allocate stack space and process function args
    // arm allowed 4 regs to transport function args, so the rest should be stored in mem.
    int cnt = 0;
    std::vector<Value *> source, target;
    Type temp_type(Type::IntegerTy32ID,module);
    std::vector<Value> temp(func->get_args().size(), Value(&temp_type));
    for (auto &arg : func->get_args()) {
        bool extended = false;
        auto sizeof_val = arg->get_type()->get_size();
        sizeof_val = ((sizeof_val + 3) / 4) * 4;
        auto dummy = &temp.at(cnt);
        if (cnt >= 4) {
            int offset = stack_size + (save_register.size() + cnt - 4) * 4;
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
    asm_code += codegen::virtualRegMove(target, source, 0);
    return asm_code;
}

std::string codegen::generateFunctionExitCode(Function *func) {
    std::string asm_code;
    asm_code+=InstGen::gen_add(InstGen::Reg(13),InstGen::Reg(13),InstGen::Constant(this->stack_size));
    auto save_register=codegen::getCalleeSaveRegisters(func);
    save_register.push_back(InstGen::Reg(15)); //pc
    std::sort(save_register.begin(),save_register.end());
    asm_code+=InstGen::gen_pop(save_register);
    return asm_code;
}

std::string codegen::generateFunctionPostCode(Function *func) {
    std::string asm_code;
    asm_code+=codegen::getFunctionLabelName(func,1)+":"+newline;
    return asm_code;
}

std::string codegen::generateBasicBlockCode(BasicBlock *bb) {
    std::string asm_code;
    asm_code+=codegen::getBasicBlockLabelName(bb)+":"+newline;
    for (auto instr:bb->get_instructions()){
        asm_code+=codegen::generateInstructionCode(instr);
    }
    return asm_code;
}

std::string codegen::generateInstructionCode(Instruction *instr) {
    std::string asm_code;
    auto &ops=instr->get_operands();
    if (instr->is_ret()){
        if (ops.size()==0){
            asm_code+=codegen::assignSpecificReg(ops[0],0,0);
        }
        else{
           // asm_code+=codegen::assignSpecificReg(ops[0],ops[0]);
        }
        asm_code+=codegen::generateFunctionExitCode(instr->get_function());
    }
    else if (instr->is_load()){
        if (codegen::reg_value_table[codegen::reg_mapping[instr]]!=nullptr &&
            codegen::reg_value_table[codegen::reg_mapping[instr]]!=instr){
            //asm_code+=InstGen::gen_str(codegen::reg_value_table[codegen::reg_mapping[instr]], stack_mapping[instr]);
        }
        //asm_code+=InstGen::gen_ldr(codegen::reg_mapping[instr],codegen::stack_mapping[instr]);
    }
    else if(instr->is_store()){
        //asm_code+=InstGen::gen_str(codegen::reg_mapping[instr],stack_mapping[instr]);
    }
    else if(instr->isBinary()){
        auto target=codegen::reg_mapping[instr];
        auto lval=reg_mapping[instr->get_operand(0)];
        //auto op1=codegen::reg_mapping[lval];
        ///auto st1=stack_mapping[lval];
        auto rval=instr->get_operand(1);
        /*if (rval->get_type()->is_integer_type()){
            if (reg_value_table[op1]== nullptr){
                asm_code+=InstGen::gen_ldr(op1,st1);
            }
            else if (reg_value_table[op1]!=lval){
                asm_code+=InstGen::gen_str(op1,stack_mapping[])
            }
        }*/
    }
}

std::string codegen::globalVariable(std::string name) {
    return tab+".global "+name;
}

std::string codegen::getBasicBlockLabelName(BasicBlock *bb) {
    return "."+bb->get_name();
}

std::string codegen::getFunctionLabelName(Function *func, int type) {
    return "."+func->get_name();
}


std::string codegen::generateFunctionCall(Instruction *instr, std::string func_name, std::vector<Value *> oprands, int return_reg, int offsets) {
    std::string asm_code;
    auto cur_func=instr->get_function();
    auto save_register=codegen::getCallerSaveRegisters(cur_func);
    if (!save_register.empty()){
        asm_code+=InstGen::gen_push(save_register);
    }
    int saved_size=save_register.size()*4;
    int args_size=0;
    bool returned=(return_reg>0) && (instr->get_type()->get_size()>0);
    if (returned && this->reg_mapping.count(instr)) {
        auto returned_reg = InstGen::Reg(this->reg_mapping.at(instr));
        decltype(save_register) new_save_registers;
        for (auto &reg : save_register) {
            if (reg.getID() != returned_reg.getID()) {
                new_save_registers.push_back(reg);
            }
        }
        save_register = new_save_registers;
    }
    std::vector<Value *>source,target;
    {
        std::vector<Value *> source, target;
        Type temp_type(Type::IntegerTy32ID,module);
        std::vector<Value> dummys(oprands.size(), Value(&temp_type,""));
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
                args_size += 4;
                auto dummy = &dummys.at(i);
                this->reg_mapping.erase(dummy);
                this->stack_mapping[dummy] = -(args_size + saved_size);
                target.push_back(dummy);
                source.push_back(oprands.at(i));
            }
        }
        asm_code += codegen::virtualRegMove(target, source, saved_size + offsets);
    }
    asm_code += InstGen::gen_sub(InstGen::Reg(13), InstGen::Reg(13),InstGen::Constant(args_size));
    asm_code += InstGen::gen_bl(func_name);
    asm_code += InstGen::gen_add(InstGen::Reg(13), InstGen::Reg(13),InstGen::Constant(args_size));
    if (returned) {
        asm_code +=
                codegen::getSpecificReg(instr, return_reg, saved_size + offsets);
    }
    if (!save_register.empty()) {
        asm_code += InstGen::gen_pop(save_register);
    }
    return asm_code;
}

std::vector<InstGen::Reg> codegen::getAllRegisters(Function *func) {
    std::set<InstGen::Reg> registers;
    for (auto &arg: func->get_args()){
        if (this->reg_mapping.count(arg) && this->reg_mapping.at(arg)<=InstGen::max_reg_id){
            registers.insert(InstGen::Reg(this->reg_mapping.at(arg)));
        }
    }
    for (auto &bb: func->get_basic_blocks()){
        for (auto &instr: bb->get_instructions()){
            if (reg_mapping.count(instr) && this->reg_mapping.at(instr)<=InstGen::max_reg_id){
                registers.insert(InstGen::Reg(this->reg_mapping.at(instr)));
            }
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(),registers.end());
}

std::vector<InstGen::Reg> codegen::getCallerSaveRegisters(Function *func){
    std::vector<InstGen::Reg> registers;
    for (auto &reg: codegen::getAllRegisters(func)){
        if (caller_save_regs.count(reg)){
            registers.push_back(reg);
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(),registers.end());
}
std::vector<InstGen::Reg> codegen::getCalleeSaveRegisters(Function *func) {
    std::vector<InstGen::Reg> registers;
    for (auto &reg: codegen::getAllRegisters(func)){
        if (callee_save_regs.count(reg)){
            registers.push_back(reg);
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(),registers.end());
}


void codegen::allocateStackSpace(Function *func) {
    this->stack_size = 0;
    this->stack_mapping.clear();
    for (auto &arg : func->get_args()) {
        auto sizeof_val = arg->get_type()->get_size();
        sizeof_val = ((sizeof_val + 3) / 4) * 4;
        if (!reg_mapping.count(arg)) {
            stack_mapping[arg] = this->stack_size;
            this->stack_size += sizeof_val;
        }
    }
    // non alloca space and non alloca pointer
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &inst : bb->get_instructions()) {
            if (this->reg_mapping.count(inst)) {
                int map_reg_id = this->reg_mapping.at(inst);
                if (map_reg_id > InstGen::max_reg_id) {
                    reg_mapping.erase(inst);
                }
            }
            if (this->reg_mapping.count(inst)) {
                continue;
            }
            if (this->stack_mapping.count(inst)) {
                continue;
            }
            if (inst->is_alloca()) {
                continue;
            }
            auto sizeof_val = inst->get_type()->get_size();
            sizeof_val = ((sizeof_val + 3) / 4) * 4;
            if (sizeof_val > 0) {
                this->stack_mapping[inst] = this->stack_size;
                this->stack_size += sizeof_val;
            }
        }
    }
    // alloca space
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &inst : bb->get_instructions()) {
            if (!inst->is_alloca()) {
                continue;
            }
            this->allocated.insert(inst);
            auto sizeof_val = inst->get_type()->get_size();
            sizeof_val = ((sizeof_val + 3) / 4) * 4;
            if (sizeof_val > 0) {
                this->stack_mapping[inst] = this->stack_size;
                this->stack_size += sizeof_val;
            }
        }
    }
}

bool codegen::isSameMapping(Value *a, Value *b) {
    if (reg_mapping.count(a) && reg_mapping.count(b))
        return reg_mapping.at(a)==reg_mapping.at(b);
    if (stack_mapping.count(a) && stack_mapping.count(b))
        return stack_mapping.at(a)==stack_mapping.at(b);
    return false;
}

std::string codegen::virtualRegMove(std::vector<Value *> target, std::vector<Value *> source, int offsets) {
    std::string asm_code;
    assert(target.size() == source.size());
    int sz = target.size();
    std::list<std::pair<Value *, Value *>> L;
    for (int i = 0; i < sz; i++) {
        L.push_back({target.at(i), source.at(i)});
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
                asm_code += codegen::virtualRegMove(it->first, it->second, offsets);
                L.erase(it);
                flag = false;
                break;
            }
        }
        if (flag) {
            if (tg_val != nullptr) {
                asm_code += codegen::getSpecificReg(tg_val, op_reg_0,offsets);
            }
            auto it = L.begin();
            asm_code += codegen::assignSpecificReg(it->second, op_reg_0, offsets);
            tg_val = it->first;
            L.erase(it);
        }
    }
    if (tg_val != nullptr) {
        asm_code += codegen::getSpecificReg(tg_val, op_reg_0);
    }
    return asm_code;
}

std::string codegen::virtualRegMove(Value *target, Value *source, int offsets) {
    std::string asm_code;
    if (codegen::isSameMapping(target, source)) {
        return asm_code;
    }
    int alu_op0 = this->reg_mapping.count(target) ? this->reg_mapping.at(target) : op_reg_0;
    asm_code += codegen::assignSpecificReg(source, alu_op0);
    asm_code += codegen::getSpecificReg(target, alu_op0, offsets);
    return asm_code;
}


std::string codegen::assignSpecificReg(Value *val, int target, int offsets) {
    auto val_const=dynamic_cast<ConstantInt *>(val);
    auto val_global=dynamic_cast<GlobalVariable *>(val);
    std::string asm_code;
    if (val_const){
        int imm=val_const->get_value();
        return InstGen::gen_set_value(InstGen::Reg(target),InstGen::Constant(imm));
    }
    else if (val_global){
        asm_code+=InstGen::gen_adrl(InstGen::Reg(op_reg_2),InstGen::Label(global_vars_label,codegen::getGlobalAddress(val_global)*4));
        asm_code += InstGen::gen_ldr(InstGen::Reg(target), InstGen::Addr(InstGen::Reg(op_reg_2), 0));
        return asm_code;
    }
    else if (reg_mapping.count(val) &&
              reg_mapping.at(val) <= InstGen::max_reg_id) {
        auto source = reg_mapping.at(val);
        asm_code += InstGen::gen_mov(InstGen::Reg(target), InstGen::Reg(source));
        return asm_code;
    } else if (allocated.count(val)) {
        auto offset = stack_mapping.at(val) + offsets;
        asm_code += InstGen::gen_add(InstGen::Reg(target),InstGen::Reg(13), InstGen::Constant(offset));
        return asm_code;
    } else if (stack_mapping.count(val)) {
        auto offset = stack_mapping.at(val) + offsets;
        asm_code += InstGen::gen_ldr(InstGen::Reg(target),InstGen::Addr(InstGen::Reg(13), offset));
        asm_code += InstGen::gen_ldr(InstGen::Reg(target), InstGen::Reg(13),InstGen::Reg(op_reg_2));
        return asm_code;
    } else {
        std::cerr << "Function assignToSpecificReg exception!" << std::endl;
        abort();
    }
}

std::string codegen::getSpecificReg(Value *val, int source, int offsets) {
    std::string asm_code;
    if (reg_mapping.count(val) &&
        reg_mapping.at(val) <= InstGen::max_reg_id) {
        auto target = reg_mapping.at(val);
        asm_code += InstGen::gen_mov(InstGen::Reg(target), InstGen::Reg(source));
    }
    else if (stack_mapping.count(val)) {
        auto offset = stack_mapping.at(val) + offsets;
        asm_code += InstGen::gen_str(InstGen::Reg(source), InstGen::Addr(InstGen::Reg(13), offset));
    }
    return asm_code;
}

int codegen::getGlobalAddress(Value *val) {
    return global_table.at(val);
}

std::string codegen::generateGlobalVarsCode() {
    std::string asm_code;
    for (auto &global_var : this->module->get_global_variable()) {
        asm_code += global_var->get_name() + ":" + InstGen::newline;
        if (!Type::is_eq_type(global_var->get_type()->get_pointer_element_type(),global_var->get_operands().at(0)->get_type())) {
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
    auto array_init=dynamic_cast<ConstantArray *>(init);
    if (array_init){
        auto length=static_cast<ArrayType*>(array_init->get_type())->get_num_of_elements();
        for (int i=1;i<=length;i++)
            asm_code+=codegen::generateInitializerCode(array_init->get_element_value(i));
    }
    else{
        auto const_init=dynamic_cast<ConstantInt *> (init);
        asm_code+=tab+".long "+std::to_string(const_init->get_value())+newline;
    }
    return asm_code;
}


std::string codegen::comment(std::string s) {
    return tab+"@ "+s+newline;
}

std::map<Value *, int> codegen::regAlloc() {
    return std::map<Value *, int>();
}

std::string codegen::generateGlobalTable(){
    std::string asm_code;
    this->global_table.clear();
    for (auto &global_var: this->module->get_global_variable()){
        int count=this->global_table.size();
        if (!this->global_table.count(global_var)){
            this->global_table[global_var]=count;
        }
    }
    std::vector<Value *> vector_got;
    vector_got.resize(this->global_table.size());
    for (auto &i: global_table){
        vector_got[i.second]=i.first;
    }
    for (auto &i: vector_got){
        asm_code+=tab+".global "+i->get_name()+newline;
    }
    asm_code+=global_vars_label+":"+newline;
    for (auto &i: vector_got){
        asm_code+=tab+".long "+i->get_name()+newline;
    }
}
std::string codegen::runCodeGenerate() {
    std::string asm_code;
    asm_code.clear();
    asm_code+= generateModuleCode();
    return asm_code;
}
