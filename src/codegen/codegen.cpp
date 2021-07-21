//
// Created by mskhana on 2021/7/20.
//

#include "include/codegen/codegen.h"

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
    asm_code+=codegen::mt_start();
    asm_code+=codegen::mt_end();
    asm_code+=tab+".data"+newline;
    asm_code+=generateGlobalVarsCode();
    return asm_code;
}

std::string codegen::generateFunctionCode(Function *func) {
    std::string asm_code;
    asm_code+=codegen::mt_vars(func);
    asm_code+=codegen::allocate_stack(func);
}

std::string codegen::generateFunctionEntryCode(Function *func) {
    return std::string();
}

std::string codegen::generateFunctionExitCode(Function *func) {
    return std::string();
}

std::string codegen::generateFunctionPostCode(Function *func) {
    return std::string();
}

std::string codegen::generateBasicBlockCode(BasicBlock *bb) {
    return std::string();
}

std::string codegen::generateInstructionCode(Instruction *instr) {
    return std::string();
}

std::string codegen::globalVariable(std::string name) {
    return tab+".global "+name+newline;
}

std::string codegen::getBasicBlockLabelName(BasicBlock *bb) {
    return std::string();
}

std::string codegen::getFunctionLabelName(Function *func, int type) {
    return std::string();
}


codegen::generateFunctionCall(Instruction *instr, std::string func_name, std::vector<Value *> oprands, int return_reg,
                              int sp_ofs) {
    return std::string();
}

std::vector<InstGen::Reg> codegen::getAllRegisters(Function *func) {
    return std::string();
}

std::vector<InstGen::Reg> codegen::getCallerSaveRegisters(Function *func) {
    return nullptr;
}

std::vector<InstGen::Reg> codegen::getCalleeSaveRegisters(Function *func) {
    return nullptr;
}

void codegen::allocateStackSpace(Function *func) {

}

bool codegen::isSameMapping(Value *a, Value *b) {
    return false;
}

std::string codegen::virtualRegMove(std::vector<Value *> target, std::vector<Value *> source) {
    return std::string();
}

std::string codegen::virtualRegMove(Value *target, Value *sourse) {
    return std::string();
}


std::string codegen::assignSpecificReg(Value *val, int target) {
    return std::string();
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

