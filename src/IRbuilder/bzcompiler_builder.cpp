//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"
#include <list>
#include <stack>
Value *ret;
Function *currentfunc;
std::list <BasicBlock *> orUnfilledTrueList;
std::list <BasicBlock *> andExitList;

BasicBlock *orTrueExit;
BasicBlock *andFalseExit;

std::stack <BasicBlock* > curIteration;
std::stack <BasicBlock* > curIterationExit;
std::stack <BasicBlock* > curIterationJudge;
BasicBlock *curIteration;
BasicBlock *curIterationExit;
BasicBlock *curIterationJudge;
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
void ASTvisitor::visit(ASTExpressionStmt &node) {
    if (node.isValidExpression())
        node.getExpression()->accept(*this);
}
void ASTvisitor::visit(ASTIfStmt &node) {
    auto tmp=builder->get_insert_block();
    if (node.hasElseStatement()){
        auto trueBB=BasicBlock::create(module.get(),"",currentfunc);
        auto falseBB=BasicBlock::create(module.get(),"",currentfunc);
        auto exitBB=BasicBlock::create(module.get(),"",currentfunc);
        orTrueExit=trueBB;
        builder->set_insert_point(tmp);
        node._condition->accept(*this);
        builder->create_cond_br(ret,trueBB,falseBB);
        builder->set_insert_point(trueBB);
        node.true_stmt->accept(*this);
        bool isReturned=true;
        if (builder->get_insert_block()->get_terminator()==nullptr){
            builder->create_br(exitBB);
            isReturned=false;
        }
        builder->set_insert_point(falseBB);
        node.false_stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator()==nullptr){
            builder->create_br(exitBB);
            isReturned=false;
        }
        if (!isReturned){
            currentfunc->add_basic_block(exitBB);
            builder->set_insert_point(exitBB);
        }
    }
    else{
        auto trueBB=BasicBlock::create(module.get(),"",currentfunc);
        auto exitBB=BasicBlock::create(module.get(),"",currentfunc);
        orTrueExit=trueBB;
        builder->set_insert_point(tmp);
        node._condition->accept(*this);
        builder->create_cond_br(ret,trueBB,exitBB);
        builder->set_insert_point(trueBB);
        node.true_stmt->accept(*this);
        bool isReturned=true;
        if (builder->get_insert_block()->get_terminator()==nullptr){
            builder->create_br(exitBB);
            isReturned=false;
        }
        if (!isReturned){
            currentfunc->add_basic_block(exitBB);
            builder->set_insert_point(exitBB);
        }
    }
    orTrueExit=nullptr;
}
void ASTvisitor::visit(ASTWhileStmt &node) {
    auto tmp=builder->get_insert_block();
    auto judgebb=BasicBlock::create(module.get(),"",currentfunc);
    auto iteratebb=BasicBlock::create(module.get(),"",currentfunc);
    auto exitbb=BasicBlock::create(module.get(),"",currentfunc);
    builder->set_insert_point(tmp);
    builder->create_br(judgebb);
    builder->set_insert_point(judgebb);
    orTrueExit=iteratebb;
    node._cond->accept(*this);
    builder->create_cond_br(ret,iteratebb,exitbb);

    builder->set_insert_point(iteratebb);
    curIteration.push(iteratebb);
    curIterationExit.push(exitbb);
    curIterationJudge.push(judgebb);

    node._while_stmt->accept(*this);
    builder->create_br(judgebb);

    builder->set_insert_point(exitbb);
    curIteration.pop();
    curIterationExit.pop();
    curIterationJudge.pop();
}
void ASTvisitor::visit(ASTBreakStmt &node) {
    builder->create_br(curIterationExit.top());
}
void ASTvisitor::visit(ASTContinueStmt &node) {
    builder->create_br(curIterationJudge.top());
}
