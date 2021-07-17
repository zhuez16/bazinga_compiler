//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"
// type
Type *TyInt32;
Type *TyVoid;
Type *TyInt32Ptr;
Type *TyInt1;
// store temporary value
Value *tmp_val = nullptr;
// whether require lvalue
bool require_address = false;
// function that is being built
Function *cur_fun = nullptr;
// detect scope pre-enter (for elegance only)
bool pre_enter_scope = false;
//
std::vector<Value *> array_init;
//
// std::vector<BasicBlock*> iter_expr,iter_cont;
enum CurBaseListType { WHILE_COND, WHILE_BODY, IF_COND, IF_THEN, IF_ELSE };

std::vector<CurBaseListType> BL_types;

std::vector<BaseBlock *> base_layer;
int tmp_int = 0;
bool use_int = false;
bool in_global_init = false;
Value *ret;
void ASTvisitor::visit(ASTProgram &node) {
    for(auto delc: node.getDeclareList()){
        delc->accept(*this);
    }
}
void ASTvisitor::visit(ASTConstant &node) 
{
    tmp_int = node.getValue();
}
void ASTvisitor::visit(ASTUnaryOp &node) 
{
    if (use_int) {
        int val;
        if (sub_exp) {
            node.getExpression()->accept(*this);
            val = tmp_int;
        } else {
            //_IRBUILDER_ERROR_("Function call in ConstExp!");
        }
        switch (node.getUnaryOpType()) {
            case AST_OP_POSITIVE:
                tmp_int = 0 + val;
                break;
            case AST_OP_NEGATIVE:
                tmp_int = 0 - val;
                break;
            case AST_OP_INVERSE:
                tmp_int = (val != 0);
                //_IRBUILDER_ERROR_("NOT operation in ConstExp!")
            break;
        }
        return;
    }

    Value *val;
    node.getExpression()->accept(*this);
    val = tmp_val;

    switch (node.getUnaryOpType()) {
    case AST_OP_POSITIVE:
        val = builder->create_iadd(CONST(0), val);
        break;
    case AST_OP_NEGATIVE:
        val = builder->create_isub(CONST(0), val);
        break;
    case AST_OP_INVERSE:
        val = builder->create_icmp_eq(val, CONST(0);
        break;
    }
    tmp_val = val;
}
void ASTvisitor::visit(ASTMulOp &node) 
{
    if(node.getOperand1()==nullptr)
        node.getOperand2()->accept(*this);
    else
    {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_int;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_int;
        if(use_int){
            switch (node.getOpType()) {
                case AST_OP_MUL:
                    tmp_int = l_val * r_val;
                    break;
                case AST_OP_DIV:
                    tmp_int = l_val / r_val;
                    break;
                case AST_OP_MOD:
                    tmp_int = l_val % r_val;
                    break;
            }
            return;
        }
        if (l_val->getType()->is_Int1()) {
            l_val = builder->create_zext(l_val, TyInt32);
        }

            if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }
        switch (node.getOpType()) {
            case OP_MUL:
                tmp_val = builder->create_imul(l_val, r_val);
            break;
            case OP_DIV:
                tmp_val = builder->create_isdiv(l_val, r_val);
            break;
            case OP_MOD:
                tmp_val = builder->create_irem(l_val, r_val);
            break;
        }
    }
}
void ASTvisitor::visit(ASTAddOp &node) 
{
    if(node.getOperand1()==nullptr)
        node.getOperand2()->accept(*this);
    else
    {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_int;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_int;
        if(use_int){
            switch (node.getOpType()) {
                case AST_OP_ADD:
                    tmp_int = l_val + r_val;
                    break;
                case AST_OP_MINUS:
                    tmp_int = l_val - r_val;
                    break;
            }
            return;
        }
        if (l_val->getType()->is_Int1()) {
            l_val = builder->create_zext(l_val, TyInt32);
        }

            if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }
        switch (node.getOpType()) {
            case AST_OP_ADD:
                tmp_val = builder->create_iadd(l_val, r_val);
            break;
            case AST_OP_MINUS:
                tmp_val = builder->create_isub(l_val, r_val);
            break;
        }
    }
}
void ASTvisitor::visit(ASTRelOp &node) 
{
    if (node.getOperand1() == nullptr)
        node.getOperand2()->accept(*this); 
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->getType()->is_Int1())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }

        switch (node.getOpType()) {
            case AST_OP_LTE:
                tmp_val = builder->create_icmp_le(l_val, r_val);
                break;
            case AST_OP_LT:
                tmp_val = builder->create_icmp_lt(l_val, r_val);
                break;
            case AST_OP_GT:
                tmp_val = builder->create_icmp_gt(l_val, r_val);
                break;
            case AST_OP_GTE:
                tmp_val = builder->create_icmp_ge(l_val, r_val);
                break;
        }
    }
}
void ASTvisitor::visit(ASTEqOp &node) 
{
    if (node.getOperand1() == nullptr)
        node.getOperand2()->accept(*this); 
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->getType()->is_Int1())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }

        switch (node.getOpType()) {
            case AST_OP_EQ:
                tmp_val = builder->create_icmp_eq(l_val, r_val);
                break;
            case AST_OP_NEQ:
                tmp_val = builder->create_icmp_ne(l_val, r_val);
                break;
        }
    }
}
void ASTvisitor::visit(ASTAndOp &node) 
{
    if (node.getOperand1() == nullptr)
        node.getOperand2()->accept(*this); 
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->getType()->is_Int1())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }

        tmp_val = builder->create_iand(l_val, r_val);
    }
}
void ASTvisitor::visit(ASTOrOp &node) 
{
    if (node.getOperand1() == nullptr)
    {
        node.getOperand2()->accept(*this); 
        if (tmp_val->getType()->is_Int32()) {
            tmp_val = builder->create_icmp_ne(tmp_val, CONST(0));
        }
    }    
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->getType()->is_Int1())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->getType()->is_Int1()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }

        tmp_val = builder->vreate_ior(l_val, r_val);
        if (tmp_val->getType()->is_Int32()) {
            tmp_val = builder->create_icmp_ne(tmp_val, CONST(0));
        }

    }
}
void ASTvisitor::visit(ASTLVal &node) 
{
    /*
    auto var = scope.find(node.getVarName);
    if (var->getType()->is_Integer_type()) { // constant
        tmp_val = var;
        return;
    }
    auto is_int = var->getType()->get_PtrElement_type()->is_Integer_type();
    auto is_ptr = var->getType()->get_PtrElement_type()->is_Pointer_type();
    if (node.getPointerExpression.size() == 0) {
        if (is_int) 
            tmp_val = scope.find(node.getVarName);
        else if (is_ptr) 
            tmp_val = builder->create_load(var);
        else 
            tmp_val = builder->create_gep(var, {CONST(0)});
    } 
    else 
    {
        Value *tmp_ptr;
        if (is_int) {
            tmp_ptr = var;
            for (auto exp : node.getPointerExpression) 
            {
                exp->accept(*this);
                tmp_ptr = builder->create_gep(tmp_ptr, {tmp_val});
            }
        } 
        else if (is_ptr) 
        {
            //to_do
            std::vector<Value *> array_params;
            scope.find_params(node.getVarName, array_params);
            tmp_ptr = builder->create_load(var); // array_load
            for (int i = 0; i < node.getPointerExpression.size(); i++) 
            {
                node.getPointerExpression[i]->accept(*this);
                auto val = tmp_val;
                for (int j = i + 1; j < array_params.size(); j++) 
                {
                    val = builder->create_imul(val, array_params[j]);
                }
                tmp_ptr = builder->create_gep(tmp_ptr, {val});
            }
        }
        else 
        {
            tmp_ptr = var;
            for (auto exp : node._pointer_exp) 
            {
                exp->accept(*this);
                tmp_ptr = builder->create_gep(tmp_ptr, {tmp_val});
            }    
        }
        tmp_val = tmp_ptr;
    }
    */
}
void ASTvisitor::visit(ASTFuncCall &node) 
{
    auto func_name = scope.find(node.getFunctionName());
    if (func_name == nullptr) 
        exit(120);
    std::vector<Value *> args;
    std::vector<ASTAddOp *> params=node.getParamList();
    for (int i = 0; i < size; i++) {
        auto arg = params[i];
        auto arg_type =
            static_cast<Function *>(func_name)->get_function_type()->get_args_type(i);
        if (arg_type->is_Integer_type())
            require_address = false;
        else 
            require_address = true;
        arg->accept(*this);
        require_address = false;
        args.push_back(tmp_val);
    }
    tmp_val = builder->create_call(static_cast<Function *>(func_name), args);
}
}
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
