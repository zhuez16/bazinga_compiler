//
// Created by 顾超 on 2021/7/12.
//
// 抽象语法树生成类
// 用于将BISON输出的语法树进行展平操作 (Flatten the AST)
//

#ifndef BAZINGA_COMPILER_AST_H
#define BAZINGA_COMPILER_AST_H

#include "syntax_tree.h"
#include <algorithm>

/*
 * @brief 抽象语法树入口类 \n
 * 该类提供AST的入口，每个语法分析树对应一个Program
 */

class ASTDecl {
public:
    enum ASTDeclType {
        FUNC_DECL,
        VAR_DECL
    };
private:
    ASTDeclType _type;
public:
    ASTDeclType getType() {
        return _type;
    }

    ASTDecl(ASTDeclType type) {
        this->_type = type;
    }
};

class ASTVarDecl : public ASTDecl {

public:
    enum ASTVarType {
        AST_VAR_INT,
    };

    struct ASTVarDeclInst {
        std::string var_name;
        ASTVarType var_type;
        bool array;
        int dimension;
        std::vector<TreeNode *> _array_list;
        bool has_initial;
        TreeNode* initial_node;

        ASTVarDeclInst(std::string name, ASTVarType type) {
            var_name = name;
            var_type = type;
            array = false;
            dimension = 0;
            has_initial = false;
            initial_node = nullptr;
        }

        ASTVarDeclInst(std::string name, ASTVarType type, std::vector<TreeNode *> array_list) {
            var_name = name;
            var_type = type;
            array = true;
            _array_list = array_list;
            dimension = array_list.size();
            has_initial = false;
            initial_node = nullptr;
        }

        ASTVarDeclInst(std::string name, ASTVarType type, TreeNode * init) {
            var_name = name;
            var_type = type;
            array = false;
            dimension = 0;
            has_initial = true;
            initial_node = init;
        }

        ASTVarDeclInst(std::string name, ASTVarType type, std::vector<TreeNode *> array_list, TreeNode * init) {
            var_name = name;
            var_type = type;
            array = true;
            _array_list = array_list;
            dimension = array_list.size();
            has_initial = true;
            initial_node = init;
        }
    };

private:

    std::vector<ASTVarDeclInst *> _var_list;
    ASTVarType _var_type;

    void index_walker(TreeNode *node, std::vector<TreeNode *> &ind_list) {
        ind_list.push_back(node->children[1]);
        if (node->children.size() == 4) {
            index_walker(node->children[3], ind_list);
        }
    }

    void var_decl_list_walker(TreeNode *node) {
        if (node->node_type == AST_var_defs) {
            var_decl_list_walker(node->children[0]);
            if (node->children.size() == 3) {
                var_decl_list_walker(node->children[2]);
            }
        } else if (node->node_type == AST_var_def) {
            if (node->children.size() == 1) {
                // int a;
                _var_list.push_back(new ASTVarDeclInst(node->children[0]->node_name, this->_var_type));
            } else if (node->children.size() == 2) {
                // int a[1][...];
                std::vector<TreeNode *> ind;
                index_walker(node->children[1], ind);
                _var_list.push_back(new ASTVarDeclInst(node->children[0]->node_name, this->_var_type, ind));
            } else if (node->children.size() == 3) {
                // int a = 1;
                _var_list.push_back(new ASTVarDeclInst(node->children[0]->node_name, this->_var_type, node->children[2]));
            } else {
                std::vector<TreeNode *> ind;
                index_walker(node->children[1], ind);
                _var_list.push_back(new ASTVarDeclInst(node->children[0]->node_name, this->_var_type, ind, node->children[3]));
            }
        }
    }
public:

    ASTVarDecl(TreeNode *node) : ASTDecl(VAR_DECL) {
        assert(node != nullptr && node->node_type == AST_var_decl && "ASTVarDecl got invalid TreeNode pointer");
        // 此处正常应该用children[0]进行判断，因语言特性做简略处理
        _var_type = AST_VAR_INT;

    }
};


class ASTParam {
public:
    enum ASTFuncParamType {
        AST_PARAM_INT,
    };
private:
    std::string _var_name;
    ASTFuncParamType _type;
    bool _array;
    int _dimension;
    std::vector<TreeNode *> _array_list;
public:
    ASTParam(TreeNode* t) {
        assert(t != nullptr && t->node_type == AST_funcf_param && "ASTParam got invalid TreeNode pointer.");
        // 该实现只有int类型
        _type = AST_PARAM_INT;
        _var_name = t->children[1]->node_name;
        if (t->children.size() == 2) {
            // 普通的整型变量
            _array = false;
        } else if (t->children.size() == 3) {
            // int a[] 形式的一维指针变量
            _array = true;
            _dimension = 1;
        } else {
            // int a[][const_exp]... 形式的多维指针变量
            _array = true;
            TreeNode *pointer = t->children[3];
            while (pointer->node_type == AST_pointer) {
                _array_list.push_back(pointer->children[1]);
                pointer = pointer->children[3];
                _dimension += 1;
            }
        }
    }
};


class ASTBlock {
private:
    TreeNode *_root;
public:
    explicit ASTBlock(TreeNode *node) {
        _root = node;
    }
};


class ASTFuncDecl : public ASTDecl {
public:
    enum FuncRetType {
        AST_RET_INT,
        AST_RET_VOID
    };

private:
    TreeNode *_root;
    FuncRetType _ret_type;
    std::string _func_name;
    std::vector<ASTParam *> _params;
    ASTBlock *_block;

    void walker(TreeNode *node) {
        if (node->node_type == AST_funcf_params) {
            walker(node->children[0]);
            if (node->children.size() == 3) {
                walker(node->children[2]);
            }
        } else {
            this->_params.push_back(new ASTParam(node));
        }
    }

    void parse() {
        /** func_def 孩子结点定义：
         *  children.size == 6 时
         *      [0]: 函数返回类型
         *      [1]: 函数名
         *      [3]: 函数参数列表
         *      [5]: 函数语句(Block)
         *  children.size == 5 时
         *      [0]: 函数返回类型
         *      [1]: 函数名
         *      [4]: 函数语句(Block)
         *  注：Sysy语言仅支持int型变量，故所有变量都是int类型的，直接写死
        **/
        this->_ret_type = (_root->children[0]->node_type == AST_VOID) ? AST_RET_VOID : AST_RET_INT;
        this->_func_name = _root->children[1]->node_name;
        if (_root->children.size() == 5) {
            this->_block = new ASTBlock(_root->children[4]);
        } else if (_root->children.size() == 6) {
            this->_block = new ASTBlock(_root->children[5]);
            walker(_root->children[3]);

        } else {
            assert(0 && "Number of children of node func_def should be 5 or 6.");
        }
    }

public:
    explicit ASTFuncDecl(TreeNode *node) : ASTDecl(FUNC_DECL) {
        _root = node;
        assert(node->node_type == AST_func_def && "Node passed to ASTFuncDecl is not a function decl node.");
        parse();
    }
};


class ASTProgram {
private:
    SyntaxTree *_tree;
    std::vector<ASTDecl *> declList;

    void parse() {
        TreeNode *t = _tree->root;
        declList.clear();
        t = t->children[0];  // 树根必定只有一个孩子且为comp_unit
        while (t->node_type == AST_comp_unit) {
            // 若comp_unit有2个孩子结点，则第1个必定为comp_unit，第2个为decl
            // 否则唯一的孩子结点为decl类型，而后循环将退出
            // 这样的循环遍历会导致结点顺序与程序中相反，最后执行一次inverse操作
            TreeNode *child;
            if (t->children.size() > 1) {
                child = t->children[1];
            } else {
                child = t->children[0];
            }
            if (child->node_type == AST_decl) {
                declList.push_back(new ASTVarDecl(child));
            } else if (child->node_type == AST_func_def) {
                declList.push_back(new ASTFuncDecl(child));
            }
            t = t->children[0];
        }
        std::reverse(declList.begin(), declList.end());
    }

public:
    /**
     * @brief 生成抽象语法树
     * @param tree Bison输出的低层语法树
     */
    explicit ASTProgram(SyntaxTree *tree) {
        this->_tree = tree;
        assert(tree != nullptr && tree->root != nullptr && tree->root->node_type == AST_program &&
                "Error: Origin AST is null or has a bag structure");
        parse();
    }

    /**
     * @brief 获取程序全局声明语句 \n
     * 包括函数声明与变量声明，且两者可能交替出现，对于不同类型的声明可以采用 @see ASTDecl::getType 进行判断
     * 然后使用 static_cast 转换到对应类型
     * @return 声明列表
     */
    std::vector<ASTDecl *> getDeclareList() {
        return declList;
    }
};

#endif //BAZINGA_COMPILER_AST_H
