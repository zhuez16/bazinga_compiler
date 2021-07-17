//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"
void ASTvisitor::visit(ASTProgram &node) {
    
}
void ASTvisitor::visit(ASTConstant &) {}
void ASTvisitor::visit(ASTUnaryOp &) {}
void ASTvisitor::visit(ASTMulOp &) {}
void ASTvisitor::visit(ASTAddOp &) {}
void ASTvisitor::visit(ASTRelOp &) {}
void ASTvisitor::visit(ASTEqOp &) {}
void ASTvisitor::visit(ASTAndOp &) {}
void ASTvisitor::visit(ASTOrOp &) {}
void ASTvisitor::visit(ASTLVal &) {}
void ASTvisitor::visit(ASTFuncCall &) {}
void ASTvisitor::visit(ASTStatement &) {}
void ASTvisitor::visit(ASTDecl &) {}
void ASTvisitor::visit(ASTFuncDecl &) {}
void ASTvisitor::visit(ASTVarDecl &) {}
void ASTvisitor::visit(ASTAssignStmt &) {}
void ASTvisitor::visit(ASTExpressionStmt &) {}
void ASTvisitor::visit(ASTIfStmt &) {}
void ASTvisitor::visit(ASTWhileStmt &) {}
void ASTvisitor::visit(ASTBreakStmt &) {}
void ASTvisitor::visit(ASTContinueStmt &) {}
