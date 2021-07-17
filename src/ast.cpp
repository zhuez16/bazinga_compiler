//
// Created by 顾超 on 2021/7/14.
//

#include "ast.h"
#include "syntax_tree.h"

// ============  BEGIN OF METHODS IMPLEMENT =================

void ASTProgram::runVisitor(ASTvisitor &node){
    accept(node);
}
ASTUnaryOp::ASTUnaryOp(TreeNode *t) : ASTInstruction(AST_UNARY_EXP) {
    assert(t != nullptr && t->node_type == AST_unary_exp && t->children.size() == 2 &&
           "ASTUnaryOp got invalid TreeNode pointer");
    switch (t->children[0]->children[0]->node_type) {
        case AST_ADD:
            _op_type = AST_OP_POSITIVE;
            break;
        case AST_SUB:
            _op_type = AST_OP_NEGATIVE;
            break;
        case AST_NOT:
            _op_type = AST_OP_INVERSE;
            break;
        default:
            assert(0 && "ASTUnaryOp got unknown node type");
    }
    _sub_exp = getUnaryOp(t->children[1]);
}

ASTInstruction *ASTUnaryOp::getUnaryOp(TreeNode *t) {
    switch (t->children.size()) {
        case 2:
            // Unary Expression
            return new ASTUnaryOp(t);
        case 1:
            // Primary Expression
        {
            // node 为 PrimaryExp结点
            TreeNode *node = t->children[0];
            switch (node->children[0]->node_type) {
                case AST_LPARENTHESE:
                    // PrimaryExp->( Exp )->AddExp
                    return new ASTAddOp(node->children[1]->children[0]);
                case AST_lval:
                    // PrimaryExp->LVal->Indent (Pointer ?)
                    return new ASTLVal(node->children[0]);
                case AST_number:
                    return new ASTConstant(node->children[0]->children[0]);
                case AST_IDENT:
                    return new ASTFuncCall(node);
                default:
                    assert(0 && "ASTPrimaryExp got unknown child node type");
            }
        }
            break;
        case 3:
            // func()
        case 4:
            // func(a,...)
            return new ASTFuncCall(t->children[1]);
            break;
        default:
            assert(0 && "ASTUnaryOp got unknown sub node type");
    }
}

ASTStatement *ASTStatement::getASTStatement(TreeNode *t) {
    assert(t != nullptr && (t->node_type == AST_stmt || t->node_type == AST_decl) && "AST Statement got invalid TreeNode.");
    t = t->children[0];
    switch (t->node_type) {
        case AST_block:
            return new ASTBlock(t);
        case AST_if_stmt:
            return new ASTIfStmt(t);
        case AST_iter_stmt:
            return new ASTWhileStmt(t);
        case AST_continue_stmt:
            return new ASTContinueStmt(t);
        case AST_break_stmt:
            return new ASTBreakStmt(t);
        case AST_return_stmt:
            return new ASTReturnStmt(t);
        case AST_exp_stmt:
            return new ASTExpressionStmt(t);
        case AST_assign_stmt:
            return new ASTAssignStmt(t);
        case AST_var_decl:
        case AST_const_decl:
            return new ASTVarDecl(t);
        default:
            assert(0 && "AST Statement got unrecognizable node type.");
    }
}

// ============  END OF METHODS IMPLEMENT =================



// ============  BEGIN OF VISITOR PATTERN =================
#include <Type.h>
#include <Value.h>
void ASTvisitor::visit(ASTProgram &node) {
    for(auto delc: node.getDeclareList()){
        delc->accept(this);
    }
}

void ASTvisitor::visit(ASTDecl &node){

}

void ASTvisitor::visit(ASTFuncDecl &node){
    auto ret_type = node.getFunctionType();
    Type* fun_ret_type;
    if(ret_type == node.AST_RET_INT){
        func_type = Type::get_Int32_type();
    }
    else{
        func_type = Type::get_Void_type();
    }
    auto params = node.getParams();
    std::vector<Type *> args;
    std::vector<Value *> fun_args;
    for(auto param : params){
        if(param->isArray()){
            args.push_back(Type::get_Int32Ptr_type());
            
        }
        else{
            args.push_back(Type::get_Int32_type());
            
        }
    }
    auto fun_type = FunctionType::get(fun_ret_type, args);
    
}

void ASTvisitor::visit(ASTParam &node){

}