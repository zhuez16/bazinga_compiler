//
// Created by 顾超 on 2021/7/12.
//

/*
 * Bison输出结果生成语法树的辅助类
 */

#ifndef BAZINGA_COMPILER_SYNTAX_TREE_H
#define BAZINGA_COMPILER_SYNTAX_TREE_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

typedef struct TreeNode {
    struct TreeNode *parent;
    struct std::vector<struct TreeNode *> children;
    std::string node_name;

public:
    TreeNode() {
        this->parent = NULL;
        this->children = std::vector<struct TreeNode *>();
        this->node_name = "Anonymous";
    }

    TreeNode(const std::string name) {
        this->parent = NULL;
        this->children = std::vector<struct TreeNode *>();
        this->node_name = name;
    }

    TreeNode(const char *name) {
        this->parent = NULL;
        this->children = std::vector<struct TreeNode *>();
        this->node_name = name;
    }

    void set_parent(TreeNode *p) {
        this->parent = p;
    }

    void add_child(struct TreeNode *c) {
        if (c != NULL) {
            this->children.push_back(c);
            c->set_parent(this);
        }
    }

    void set_name(std::string name) {
        this->node_name = name;
    }

    void set_name(const char *name) {
        this->node_name = name;
    }

    void remove_child(TreeNode *c) {
        if (c != NULL) {
            std::remove(this->children.begin(), this->children.end(), c);
            c->set_parent(NULL);
        }
    }

    void print_tree(int depth) {
        std::string result = "";
        for (int i = 0; i < depth; ++i){
            result += '|';
        }
        result += this->node_name;
        std::cout << result << std::endl;
        for (TreeNode* node: this->children){
            node->print_tree(depth + 1);
        }
    }
} TreeNode;

struct SyntaxTree {
    TreeNode *root;

    void print_tree() {
        root->print_tree(0);
    }
};


#endif //BAZINGA_COMPILER_SYNTAX_TREE_H
