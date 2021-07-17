//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"

Value *ret;
void ASTvisitor::visit(ASTProgram &node) {
    for(auto delc: node.getDeclareList()){
        delc->accept(*this);
    }
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
void compute_ast_constant(){};
void ASTvisitor::visit(ASTVarDecl &node) {
    for (ASTVarDecl::ASTVarDeclInst *it: node.getVarDeclList()) {

    }

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
    auto fun =  Function::create_function(node.getFunctionName(), module, fun_type);
    auto bb = BasicBlock::create(module, "entry", fun);
    builder->set_insert_point(bb);
    scope.push(node.getFunctionName(), fun);
    scope.enter()
    for(auto param: params){
        param->accept(*this);
    }
    auto block = node.getStmtBlock()
    block->accept(*this)
    scope.exit()
}

void ASTvisitor::visit(ASTParam &node){
    if(node.isArray()){
        auto array_alloca = builder->create_alloca(Type::get_Int32Ptr_type());
        Value* arg = new Value(Type::get_Int32Ptr_type(), node.getParamName())
        builder->create_store(arg, array_alloc);
        std::vector<Value *> array_params;
        array_params.push_back(ConstantInt::get(0));
        for (auto array_param : node.getArrayList) {
            array_param->accept(*this);
            array_params.push_back(ret);
        }
        scope.push(node.getParamName(), array_alloc);
        scope.push_params(node.getParamName, array_alloc, array_params);
    }
    else{
        auto alloca = builder->create_alloca(Type::get_Int32_type());
        auto params = node.getArrayList();
        Value* arg = new Value(Type::get_Int32_type(), node.getParamName())
        builder.create_store(arg, alloca);
        scope.push(node.getParamName(), alloca)
    }
}

void ASTvisitor::visit(ASTAssignStmt &node) {
    node->_l_val->accept(*this);
    auto assign_addr=ret;
    node->_r_val->accept(*this);
    auto assign_value=ret;
    if (assign_addr->get_type()->is_pointer_type())
        assign_value=assign_addr->create_load(assign_value);
}

void ASTvisitor::visit(ASTBlock &node) {
    for(auto stmt: node.getStatements()){
        stmt->accept(*this);
    }
}

void ASTvisitor::visit(ASTExpressionStmt &node) {}
void ASTvisitor::visit(ASTIfStmt &node) {}
void ASTvisitor::visit(ASTWhileStmt &node) {}
void ASTvisitor::visit(ASTBreakStmt &node) {}
void ASTvisitor::visit(ASTContinueStmt &node) {}
