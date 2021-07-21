//
// Created by mskhana on 2021/7/20.
//

#include "include/codegen/codegen.h"


std::string codegen::generateModuleCode(std::map<Value *, int> register_mapping, bool auto_alloc) {
    std::string asm_code;
}

std::string codegen::generateFunctionCode(Function *func) {
    return std::string();
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

std::string codegen::global(std::string name) {
    return std::string();
}

std::string codegen::getBasicBlockLabelName(BasicBlock *bb) {
    return std::string();
}

std::string codegen::getFunctionLabelName(Function *func, int type) {
    return std::string();
}

std::string
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

std::string codegen::runCodeGenerate() {
    std::string asm_code;
    asm_code.clear();
    for (auto *func:module->get_functions()){
        asm_code+=generateFunctionCode(func);
    }
    return asm_code;
}

