//
// Created by 顾超 on 2021/7/12.
//

#include "syntax_tree.h"
#include "parser.h"
// #include "ast.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: parser /path/to/.sysy/file\n");
        return 0;
    }

    SyntaxTree *tree = parse(argv[1]);
    tree->print_tree();
}