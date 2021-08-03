//
// Created by 顾超 on 2021/7/29.
//

#include <iostream>
#include <fstream>
#include <string>

#include "ast.h"
#include "parser.h"
#include "bzcompiler_builder.hpp"
#include "pass_manager.h"
#include "pass/mem2reg.h"
#include "pass/GVN.h"
#include "pass/SCPcombineDCE.h"
#include "pass/CodeElimination.h"
#include "pass/Sink.h"
#include "pass/global2local.h"

#include "codegen/codegen.h"


int main(int argc, char *argv[]) {
    int i = 1;

    std::string input_filepath;
    std::string output_filepath;
    bool generate_ir = false;
    bool generate_assemble = false;
    bool generate_executable = true;
    bool apply_optimization = false;

    while (i < argc) {
        std::string arg = argv[i];
        if (arg == "-emit-llvm") {
            generate_ir = true;
        } else if (arg == "-S") {
            generate_assemble = true;
        } else if (arg == "-o") {
            output_filepath = argv[++i];
        } else if (arg == "-O2") {
            apply_optimization = true;
        }
        else {
            if (input_filepath.empty()) {
                input_filepath = arg;
            } else {
                std::cout << "Unknown param: " << arg << std::endl;
            }
        }
        ++i;
    }
    if (input_filepath.empty()) {
        std::cout << "Error: Input filepath not specified " << std::endl;
        return 0;
    }
    if (output_filepath.empty()) {
        std::cout << "Error: Output filepath not specified " << std::endl;
        return 0;
    }
    // Parser
    SyntaxTree *st = parse(input_filepath.c_str());
    // AST
    auto *ast = new ASTProgram(st);
    // Builder
    BZBuilder builder;
    ast->accept(builder);
    Pass_manager pm(builder.getModule());
    // We always need to apply mem2reg
    pm.add_pass<Mem2Reg>();
    // O2 optimizations
    if (apply_optimization) {
        pm.add_pass<Global2Local>();
        // Code elimination
        pm.add_pass<ConstFoldingDCEliminating>();
        pm.add_pass<CodeElimination>();
        // GVN
        pm.add_pass<GVN>();
        pm.add_pass<CodeElimination>();
        // Simplify CFG ?

        // Code sink
        pm.add_pass<CodeSinking>();
        pm.add_pass<CodeElimination>();
    }
    pm.run();
    // Now we can print the llvm ir code if needed
    if (generate_ir) {
        std::ofstream llvmIRStream;
        llvmIRStream.open(output_filepath, std::ios::out);
        llvmIRStream << "; ModuleID = 'sysy2021_bzcompiler'\n";
        llvmIRStream << "source_filename = \""+ input_filepath +"\"\n\n";
        llvmIRStream << builder.getModule()->print();
        llvmIRStream.close();
    }
    // Generate assembly if needed
    if (generate_assemble) {
        codegen temp=codegen(builder.getModule());
        std::map<Value *, int> reg_alloc;
        reg_alloc=temp.regAlloc();
/*
        for (auto val: reg_alloc){
            std::cout << val.first->get_name() << " " << val.second << std::endl;
        }
*/
        std::string asm_code=temp.generateModuleCode(reg_alloc);
        std::ofstream output_asm_stream;
        output_asm_stream.open(output_filepath, std::ios::out);
        output_asm_stream << asm_code;
        output_asm_stream.close();
    }
}