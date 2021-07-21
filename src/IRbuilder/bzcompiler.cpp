#include "bzcompiler_builder.hpp"
#include "parser.h"
#include "ast.h"
#include "pass/mem2reg.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>

void print_help(const std::string& exe_name) {
    std::cout << "Usage: " << exe_name <<
        " [ -h | --help ] [ -o <target-file> ] [ -emit-llvm ] [-mem2reg] [-loop-search] [-loop-inv-hoist] [-const-propagation] [-active-vars] [-available-expression] [-analyze] <input-file>" << std::endl;
}

int main(int argc, char **argv) {
    std::string target_path;
    std::string input_path;
    bool emit = false;
    bool analyze = false;
    bool mem2reg = false;
    bool const_propagation = false;
    bool activevars = false;
    bool loop_inv_hoist = false;
    bool loop_search = false;
    bool availableexpression = false;

    for (int i = 1;i < argc;++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (target_path.empty() && i + 1 < argc) {
                target_path = argv[i + 1];
                i += 1;
            } else {
                print_help(argv[0]);
                return 0;
            }
        } else if (strcmp(argv[i], "-emit-llvm") == 0) {
            emit = true;
        } else if (strcmp(argv[i], "-analyze") == 0) {
            analyze = true;
        } else if (strcmp(argv[i], "-mem2reg") == 0) {
            mem2reg = true;
        } else if (strcmp(argv[i], "-loop-search") == 0) {
            loop_search = true;
        } else if (strcmp(argv[i], "-loop-inv-hoist") == 0) {
            loop_inv_hoist = true;
        } else if (strcmp(argv[i], "-const-propagation") == 0) {
            const_propagation = true;
        } else if (strcmp(argv[i], "-active-vars") == 0) {
            activevars = true;
        } else if (strcmp(argv[i], "-available-expression") == 0){
            availableexpression = true;
        } else {
            if (input_path.empty()) {
                input_path = argv[i];
            } else {
                print_help(argv[0]);
                return 0;
            }
        }
    }
    if (input_path.empty()) {
        print_help(argv[0]);
        return 0;
    }

    if (target_path.empty()) {
        auto pos = input_path.rfind('.');
        if (pos == std::string::npos) {
            std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
            return -1;
        } else {
            if (input_path.substr(pos) != ".sy") {
                std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
                return -1;
            }
            if (emit) {
                target_path = input_path.substr(0, pos);
            } else {
                target_path = input_path.substr(0, pos);
            }
        }
    }
//    printf("start build ast\n");
    SyntaxTree *tree = parse(input_path.c_str());
    auto *ast = new ASTProgram(tree);
    BZBuilder builder;
    ast->accept(builder);
    auto m = builder.getModule();

    PassManager PM(m);
    mem2reg = true;

    m->set_print_name();
//    printf("start running pass manager\n");
    if( mem2reg )
    {
        PM.add_pass<Mem2Reg>();
    }
//    if( loop_search ){
//        PM.add_pass<LoopSearch>();
//    }
//    if( const_propagation )
//    {
//        PM.add_pass<ConstPropagation>(true);
//    }
//    if( activevars )
//    {
//        PM.add_pass<ActiveVars>();
//    }
//    if( loop_inv_hoist )
//    {
//        PM.add_pass<LoopInvHoist>(true);
//    }
//    if(availableexpression){
//        PM.add_pass<AvailableExpression>(true);
//    }
//    printf("555\n");
    PM.run();
//    PM.run();
//    printf("after running pass manager\n");
    auto IR = m->print();

    std::ofstream output_stream;
    auto output_file = target_path+".ll";
    output_stream.open(output_file, std::ios::out);
    output_stream << "; ModuleID = 'cminus'\n";
    output_stream << "source_filename = \""+ input_path +"\"\n\n";
    output_stream << IR;
    output_stream.close();
    if (!emit) {
        
        auto command_string = "clang -O0 -w " + target_path + ".ll -o " + target_path + " -L. -lsylib";
        int re_code0 = std::system(command_string.c_str());
        command_string = "rm " + target_path + ".ll";
        int re_code1 = std::system(command_string.c_str());
        if(re_code0==0 && re_code1==0) return 0;
        else return 1;
    }

    return 0;
}
