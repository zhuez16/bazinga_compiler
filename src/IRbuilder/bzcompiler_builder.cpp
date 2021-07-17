//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"

Value *ret;
void ASTvisitor::visit(ASTProgram &node) {
}
void ASTvisitor::visit(ASTConstant &node) {}
void ASTvisitor::visit(ASTUnaryOp &node) {}
void ASTvisitor::visit(ASTMulOp &node) {}
void ASTvisitor::visit(ASTAddOp &node) {}
void ASTvisitor::visit(ASTRelOp &node) {}
void ASTvisitor::visit(ASTEqOp &node) {}
void ASTvisitor::visit(ASTAndOp &node) {}
void ASTvisitor::visit(ASTOrOp &node) {}
void ASTvisitor::visit(ASTLVal &node) {}
void ASTvisitor::visit(ASTFuncCall &node) {}
//void ASTvisitor::visit(ASTStatement &) {}
void ASTvisitor::visit(ASTDecl &node) {}
void ASTvisitor::visit(ASTVarDecl &node) {}
void ASTvisitor::visit(ASTFuncDecl &node) {}
void ASTvisitor::visit(ASTAssignStmt &node) {
    node->_l_val->accept(*this);
    auto assign_addr=ret;
    node->_r_val->accept(*this);
    auto assign_value=ret;
    if (assign_addr->get_type()->is_pointer_type())
        assign_value=assign_addr->create_load(assign_value);
}
void ASTvisitor::visit(ASTExpressionStmt &node) {}
void ASTvisitor::visit(ASTIfStmt &node) {}
void ASTvisitor::visit(ASTWhileStmt &node) {}
void ASTvisitor::visit(ASTBreakStmt &node) {}
void ASTvisitor::visit(ASTContinueStmt &node) {}
