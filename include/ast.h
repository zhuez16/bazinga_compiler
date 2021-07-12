//
// Created by 顾超 on 2021/7/12.
//
// 抽象语法树生成类
// 用于将BISON输出的语法树进行展平操作 (Flatten the AST)
//

#ifndef BAZINGA_COMPILER_AST_H
#define BAZINGA_COMPILER_AST_H

#include "syntax_tree.h"

/*
 * @brief 抽象语法树入口类 \n
 * 该类提供AST的入口，每个语法分析树对应一个Program
 */
class ASTProgram {
private:
    SyntaxTree *_tree;

public:
    /**
     * @brief 生成抽象语法树
     * @param tree Bison输出的低层语法树
     */
    explicit ASTProgram(SyntaxTree *tree) {
        this->_tree = tree;
    }
};

#endif //BAZINGA_COMPILER_AST_H
