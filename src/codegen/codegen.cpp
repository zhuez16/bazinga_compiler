//
// Created by mskhana on 2021/7/20.
//

#include "include/codegen/codegen.h"
#include "include/codegen/instgen.h"
const std::string tab="    ";
const std::string newline="\n";
std::string codegen::generateModuleCode(std::map<Value *, int> register_mapping, bool auto_alloc) {
    std::string asm_code;
    asm_code+=tab+".arch armv"+std::to_string(arch_version)+"-a"+newline;
    asm_code+=tab+".file \""+"\""1+newline;
    asm_code+=tab+".text"+newline;
    for (auto func:this->module->get_functions()){
        if (func->get_basic_blocks().size())
            asm_code+=codegen::globalVariable(func->get_name());
    }
    asm_code+=tab+".arm"+newline;
    asm_code+=tab+".arm"+newline;
    asm_code+=generateGlobalTable();
    for (auto &func:this->module->get_functions()){
        if (func->get_basic_blocks().size()){
            asm_code+=codegen::generateFunctionCode(func);
        }
    }
    asm_code+=tab+".data"+newline;
    asm_code+=generateGlobalVarsCode();
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
    //clear pointer in func
    for (auto &bb:func->get_basic_blocks()){
        for (auto &inst:bb->get_instructions()){
            if (this->reg_mapping().count(inst)){
                int map_reg_id=this->reg_mapping.at(inst);
                if (map_reg_id>InstGen::max_reg_id) {
                    reg_mapping.erase(inst);
                }
            }
            if (this->reg_mapping.count(inst))
                continue;
            if (this->stack_mapping(inst))
                continue;
            if (inst->is_alloca())
                continue;
            auto sizeof_val=inst->get_type()->get_size());
            sizeof_val=((sizeof_val+3)/4)*4;
            if (sizeof_val){
                this->stack_mapping[inst]=sizeof_val;
                this->stack_size+=sizeof_val;
            }
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
std::string codegen::generateFunctionCode(Function *func) {
    std::string asm_code;
    asm_code+=codegen::allocate_stack(func);
    int counter=0;
    for (auto &bb: func->get_basic_blocks()){
        if (bb->getName().empty()){
            bb->set_name(std::to_string (counter++));
        }
    }
    asm_code+=func->get_name()+":"+newline;
    asm_code+=codegen::comment("stack_size="+std::to_string(this->stack_size));
    asm_code+=codegen::generateFunctionEntryCode();
}

std::string codegen::generateFunctionEntryCode(Function *func) {
    std::string asm_code;
    asm_code.clear();
    asm_code+=codegen::getFunctionLabelName(func,0)+":"+newline;
    asm_code+=codegen::comment("function initialize");
    auto save_registers=codegen::getCalleeSaveRegisters(func);
    save_register.push_back(InstGen::Reg(14)); //lr reg
    std::sort(save_registers.begin(),save_registers.end());
    //large stack allocate
    if (func->get_name()=="main"){
        asm_code+=InstGen::gen_push(save_registers);
        asm_code+=InstGen::
    }
    // save callee register and lr
    // allocate stack space and process function args
}

std::string codegen::generateFunctionExitCode(Function *func) {
    return std::string();
}

std::string codegen::generateFunctionPostCode(Function *func) {
    std::string asm_code;
    asm_code+=codegen::getFunctionLabelName(func,1)+":"+newline;
    return asm_code;
}

std::string codegen::generateBasicBlockCode(BasicBlock *bb) {
    return codegen::getBasicBlockLabelName(bb)+":"+newline;
}

std::string codegen::generateInstructionCode(Instruction *instr) {
    std::string asm_code;
    auto &ops=instr->get_operands();
    if (instr->is_ret()){
        if (ops.size()==0){
            asm_code+=codegen::assignSpecificReg(ops.at(0),0);
        }
        asm_code+=codegen::generateFunctionExitCode(instr->get_function());
    }
    else if (instr->is_load()){

    }
}

std::string codegen::globalVariable(std::string name) {
    return tab+".global "+name+newline;
}

std::string codegen::getBasicBlockLabelName(BasicBlock *bb) {
    return "."+bb->get_name();
}

std::string codegen::getFunctionLabelName(Function *func, int type) {
    return "."+func->get_name();
}


codegen::generateFunctionCall(Instruction *instr, std::string func_name, std::vector<Value *> oprands, int return_reg,
                              int sp_ofs) {
    return std::string();
}

std::vector<InstGen::Reg> codegen::getAllRegisters(Function *func) {
    std::set<InstGen::Reg> registers;
    for (auto &arg: func->get_args()){
        if (this->reg_mapping.count(arg) && this->reg_mapping.at(arg)<=InstGen::max_reg_id){
            registers.insert(InstGen::Reg(this->reg_mapping.at(arg)));
        }
    }

}

std::vector<InstGen::Reg> codegen::getCalleeSaveRegisters(Function *func) {
    std::vector<InstGen::Reg> registers;
    for (auto &reg: codegen::getAllRegisters(func)){
        if (callee_save_regs.count(reg)){
            register.insert(reg);
        }
    }
    return std::vector<InstGen::Reg>(registers.begin(),registers.end());
}

void codegen::allocateStackSpace(Function *func) {

}

bool codegen::isSameMapping(Value *a, Value *b) {
    return false;
}

std::string codegen::virtualRegMove(std::vector<Value *> target, std::vector<Value *> source) {
    return std::string();
}

std::string codegen::virtualRegMove(Value *target, Value *source) {
    return "MSR "+target->get_name()+","+source->get_name();
}


std::string codegen::assignSpecificReg(Reg *val, int target) {
    return "MSR "+target->get_name()+","+source->get_name();
}

std::string codegen::getSpecificReg(Value *val, int source) {
    return std::string();
}

std::string codegen::generateGlobalVarsCode() {
    return std::string();
}

std::string codegen::generateInitializerCode() {
    return std::string();
}

std::pair<int, bool> codegen::constIntVal(Value *val) {
    return std::pair<int, bool>();
}

std::string codegen::comment(std::string s) {
    return std::string();
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
    std::vector<Value &> vector_got;
    vector_got.resize(this->global_table.size());
    for (auto &i: global_table){
        vector_got[i.first]=i.second;
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
    asm_code+= generateModuleCode(module);
    return asm_code;
}
