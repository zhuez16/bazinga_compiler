//
// Created by mskhana on 2021/7/20.
//

#ifndef BAZINGA_COMPILER_CODEGEN_H
#define BAZINGA_COMPILER_CODEGEN_H
#include <linux/sched.h>
#include <sys/wait.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "IR/BasicBlock.h"
#include "IR/Constant.h"
#include "IR/Function.h"
#include "IR/GlobalVariable.h"
#include "IR/IRbuilder.h"
#include "IR/Instruction.h"
#include "IR/Module.h"
#include "IR/Type.h"
#include "IR/User.h"
#include "IR/Value.h"

const std::string global_vars_label=".global_vars";
const int arch_version=8;
const bool enlarge_stack=true;
const int enlarge_stack_size=256*(1<<20);

const int op_reg_0=12;
const int op_reg_1=14;

const int cache_line_bits=7;
const int cache_line_size=1<<cache_line_bits;
const int mt_num_threads=4;
const int L1_cache_size=32*(1<<10);
const int L2_cache_size=1*(1<<<20);
const int thread_id_reg=10;
const int clone_flag=CLONE_VM | SIGCHLD;

class codegen {
private:
    Module *module;
    std::map<Value *,int> reg_mapping;
    std::map<Value *,int> stack_mapping;
    std::set<Value *> allocated;
    std::map<Value *,int> got;
    std::map<Instruction*, std::set<Value>> context_active_vars;
    int stack_size;
    int thread_stack_bits;
    int thread_stack_size;
    bool debug;
    double spill_cost_total;
    double color_bonus_total;
    bool mt_enabled;
public:
    codegen(Module *module,bool debug=false){
        this->module=module;
        this->debug=debug;
    }
    ~codegen(){}

    std::string generateModuleCode(std::map<Value*, int> register_mapping,bool auto_alloc=true);
    std::string generateFunctionCode(Function *func);
    std::string generateFunctionEntryCode(Function *func);
    std::string generateFunctionExitCode(Function *func);
    std::string generateFunctionPostCode(Function *func);
    std::string generateBasicBlockCode(BasicBlock *bb);
    std::string generateInstructionCode(Instruction *instr);
    std::string global(std::string name);
    std::string getBasicBlockLabelName(BasicBlock *bb);
    std::string getFunctionLabelName(Function *func,int type);
    std::string generateFunctionCall(Instruction *instr,std::string func_name,
                                     std::vector<Value *>oprands,int return_reg=0,int sp_ofs=0);
    std::vector<InstGen::Reg> getAllRegisters(Function *func):
    std::vector<InstGen::Reg> getCallerSaveRegisters(Function *func);
    std::vector<InstGen::Reg> getCalleeSaveRegisters(Function *func);
    void allocateStackSpace(Function *func);
    bool isSameMapping(Value *a,Value *b);
    std::string virtualRegMove(std::vector<Value*> target,std::vector<Value*> source);
    std::string virtualRegMove(Value *target, Value *sourse);
    std::string assignSpecificReg(Value *val, int target);
    std::string getSpecificReg(Value *val, int source);
    std::string generateGlobalVarsCode();
    std::string generateInitializerCode();
    std::pair<int,bool> constIntVal(Value *val);
    std::string comment(std::string s);
    std::map<Value *, int> regAlloc();
    std::string runCodeGenerate();

};


#endif //BAZINGA_COMPILER_CODEGEN_H
