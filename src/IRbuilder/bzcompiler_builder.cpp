//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"
#include <stack>

std::stack<BasicBlock *> curIteration;
std::stack<BasicBlock *> curIterationExit;
std::stack<BasicBlock *> curIterationJudge;
Value *ret;
void BZBuilder::visit(ASTProgram &node) {
    for(auto delc: node.getDeclareList()){
        delc->accept(*this);
    }
}
void BZBuilder::visit(ASTConstant &node)
{
    tmp_int = node.getValue();
}
void BZBuilder::visit(ASTUnaryOp &node)
{
    if (use_int) {
        int val;
        if (node.getExpression()) {
            node.getExpression()->accept(*this);
            val = tmp_int;
        } else {
            //_IRBUILDER_ERROR_("Function call in ConstExp!");
        }
        switch (node.getUnaryOpType()) {
            case ASTUnaryOp::AST_OP_POSITIVE:
                tmp_int = 0 + val;
                break;
            case ASTUnaryOp::AST_OP_NEGATIVE:
                tmp_int = 0 - val;
                break;
            case ASTUnaryOp::AST_OP_INVERSE:
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
    case ASTUnaryOp::AST_OP_POSITIVE:
        val = builder->create_iadd(CONST(0), val);
        break;
    case ASTUnaryOp::AST_OP_NEGATIVE:
        val = builder->create_isub(CONST(0), val);
        break;
    case ASTUnaryOp::AST_OP_INVERSE:
        val = builder->create_icmp_eq(val, CONST(0);
        break;
    }
    tmp_val = val;
}
void BZBuilder::visit(ASTMulOp &node)
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
                case ASTMulOp::AST_OP_MUL:
                    tmp_int = l_val * r_val;
                    break;
                case ASTMulOp::AST_OP_DIV:
                    tmp_int = l_val / r_val;
                    break;
                case ASTMulOp::AST_OP_MOD:
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
            case ASTMulOp::AST_OP_MUL:
                tmp_val = builder->create_imul(l_val, r_val);
            break;
            case ASTMulOp::AST_OP_DIV:
                tmp_val = builder->create_isdiv(l_val, r_val);
            break;
            case ASTMulOp::AST_OP_MOD:
                tmp_val = builder->create_irem(l_val, r_val);
            break;
        }
    }
}
void BZBuilder::visit(ASTAddOp &node)
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
                case ASTAddOp::AST_OP_ADD:
                    tmp_int = l_val + r_val;
                    break;
                case ASTAddOp::AST_OP_MINUS:
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
            case ASTAddOp::AST_OP_ADD:
                tmp_val = builder->create_iadd(l_val, r_val);
            break;
            case ASTAddOp::AST_OP_MINUS:
                tmp_val = builder->create_isub(l_val, r_val);
            break;
        }
    }
}
void BZBuilder::visit(ASTRelOp &node)
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
            case ASTRelOp::AST_OP_LTE:
                tmp_val = builder->create_icmp_le(l_val, r_val);
                break;
            case ASTRelOp::AST_OP_LT:
                tmp_val = builder->create_icmp_lt(l_val, r_val);
                break;
            case ASTRelOp::AST_OP_GT:
                tmp_val = builder->create_icmp_gt(l_val, r_val);
                break;
            case ASTRelOp::AST_OP_GTE:
                tmp_val = builder->create_icmp_ge(l_val, r_val);
                break;
        }
    }
}
void BZBuilder::visit(ASTEqOp &node)
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
            case ASTEqOp::AST_OP_EQ:
                tmp_val = builder->create_icmp_eq(l_val, r_val);
                break;
            case ASTEqOp::AST_OP_NEQ:
                tmp_val = builder->create_icmp_ne(l_val, r_val);
                break;
        }
    }
}
void BZBuilder::visit(ASTAndOp &node)
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
void BZBuilder::visit(ASTOrOp &node)
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
void BZBuilder::visit(ASTLVal &node)
{
    std::string var_name = node.getVarName();
    std::vector<ASTAddOp *> pointer_exp= node.getPointerExpression();
    auto var = scope.find(var_name);
    if (var->getType()->is_Integer_type()) {
        tmp_val = var;
        return;
    }
    auto is_int = var->getType()->get_PtrElement_type()->is_Integer_type();
    auto is_ptr = var->getType()->get_PtrElement_type()->is_Pointer_type();
    if (pointer_exp.size() == 0) {
        if (is_int)
            tmp_val = scope.find(var_name);
        else if (is_ptr)
            tmp_val = builder->create_load(var);
        else
            tmp_val = builder->create_gep(var, {CONST(0)});
    }
    else
    {
        Value *tmp_ptr;
        if (is_ptr)
        {
            std::vector<Value *> array_params;
            scope.find_params(node.getVarName, array_params);
            tmp_ptr = builder->create_load(var);
            for (int i = 0; i < pointer_exp.size(); i++)
            {
                pointer_exp[i]->accept(*this);
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
            for (auto exp : pointer_exp)
            {
                exp->accept(*this);
                tmp_ptr = builder->create_gep(tmp_ptr, {tmp_val});
            }
        }
        tmp_val = tmp_ptr;
    }
}
void BZBuilder::visit(ASTFuncCall &node)
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

int BZBuilder::compute_ast_constant(ASTInstruction *inst) {
    switch (inst->getType()) {
        case AST_CONSTANT:
            return dynamic_cast<ASTConstant *>(inst)->getValue();
        case AST_ADD_EXP: {
            auto *op = dynamic_cast<ASTAddOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                if (op->getOpType() == ASTAddOp::AST_OP_ADD) {
                    return lhs + rhs;
                } else {
                    return lhs - rhs;
                }
            }
        }
        case AST_MUL_EXP: {
            auto *op = dynamic_cast<ASTMulOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                switch (op->getOpType()) {
                    case ASTMulOp::AST_OP_DIV:
                        return lhs / rhs;
                    case ASTMulOp::AST_OP_MUL:
                        return lhs * rhs;
                    case ASTMulOp::AST_OP_MOD:
                        return lhs % rhs;
                }
            }
        }
        case AST_OR_EXP: {
            auto *op = dynamic_cast<ASTOrOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                return lhs || rhs;
            }
        }
        case AST_AND_EXP: {
            auto *op = dynamic_cast<ASTAndOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                return lhs && rhs;
            }
        }
        case AST_EQ_EXP: {
            auto *op = dynamic_cast<ASTEqOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                switch (op->getOpType()) {
                    case ASTEqOp::AST_OP_EQ:
                        return lhs == rhs;
                    case ASTEqOp::AST_OP_NEQ:
                        return lhs != rhs;
                }
            }
        }
        case AST_REL_EXP: {
            auto *op = dynamic_cast<ASTRelOp *>(inst);
            if (op->isUnaryExp()) {
                return compute_ast_constant(op->getOperand1());
            } else {
                int lhs = compute_ast_constant(op->getOperand1());
                int rhs = compute_ast_constant(op->getOperand2());
                switch (op->getOpType()) {
                    case ASTRelOp::AST_OP_GT:
                        return lhs > rhs;
                    case ASTRelOp::AST_OP_GTE:
                        return lhs >= rhs;
                    case ASTRelOp::AST_OP_LT:
                        return lhs < rhs;
                    case ASTRelOp::AST_OP_LTE:
                        return lhs <= rhs;
                }
            }
        }
        case AST_UNARY_EXP: {
            auto *op = dynamic_cast<ASTUnaryOp *>(inst);
            int lhs = compute_ast_constant(op->getExpression());
            switch (op->getUnaryOpType()) {
                case ASTUnaryOp::AST_OP_NEGATIVE:
                    return -lhs;
                case ASTUnaryOp::AST_OP_POSITIVE:
                    return lhs;
                case ASTUnaryOp::AST_OP_INVERSE:
                    return !lhs;
            }
        }
        case AST_LVAL: {
            auto *op = dynamic_cast<ASTLVal *>(inst);
            std::string var_name = op->getVarName();
            if (op->hasAddress()) {
                std::vector<int> pointer;
                for (ASTAddOp *aop: op->getPointerExpression()) {
                    pointer.push_back(compute_ast_constant(aop));
                }
                return scope.getValue(var_name, pointer);
            }
            return scope.getValue(var_name);
        }
    }
}

/**
 * 递归数组初始化
 * @param l
 * @param offset
 * @param depth
 * @param init_values
 * @return 当前块填充的数量
 */
std::tuple<int, int>
BZBuilder::ConstInitialValueWalker(ASTVarDecl::ASTArrayList *l, const std::vector<int> &offset, int depth,
                                   std::vector<int> &init_values) {
    if (l->isEmpty) {
        for (int i = 0; i < offset[offset.size() - 1]; ++i) {
            init_values.push_back(0);
        }
        return std::make_tuple(depth + 1, offset[offset.size() - 1]);
    }
    if (l->isArray) {
        int max_depth = 0;
        int filled = 0;
        for (auto arr: l->list) {
            int _set, _depth;
            std::tie(_depth, _set) = ConstInitialValueWalker(arr, offset, depth + 1, init_values);
            max_depth = std::max(max_depth, _depth);
            filled += _set;
        }
        int delta = max_depth - depth;
        if (delta < 2) {
            // Do Nothing
        } else {
            int tb_fill = offset[offset.size() + 1 - delta];
            for (int i = 0; i < tb_fill - filled; ++i) {
                init_values.push_back(0);
            }
            return {depth, tb_fill};
        }
    } else {
        init_values.push_back(compute_ast_constant(l->value));
        return std::make_tuple(depth, 1);
    }
}

void ASTvisitor::visit(ASTVarDecl &node) {
    for (ASTVarDecl::ASTVarDeclInst *it: node.getVarDeclList()) {
        std::vector<int> dimension;
        std::vector<int> init;
        if (it->array) {
            for (ASTAddOp *op: it->_array_list) {
                dimension.push_back(compute_ast_constant(op));
            }
            if (it->has_initial) {
                ConstInitialValueWalker(it->initial_value[0], dimension, 0, init);
            }
        } else if (it->has_initial) {
            auto val = it->initial_value[0];
            if (val->isEmpty) {
                init.push_back(0);
            } else {
                init.push_back(compute_ast_constant(val->value));
            }
        }

        Type *var_ty = Type::get_int32_type(getModule().get());
        for (int i = (int)dimension.size() - 1; i >= 0; --i) {
            var_ty = new ArrayType(var_ty, dimension[i]);
        }

        if (scope.in_global()) {
            if (node.isConst()) {
                if (it->array) {
                    Constant *initializer = (Constant *) new ConstantArrayInitializer(it->var_name, dimension, init);
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule().get(),
                            var_ty,
                            true,
                            initializer
                    );
                    scope.push(it->var_name, var, true, dimension, init);
                } else {
                    auto *initializer = (Constant *) new ConstantInt(init[0]);
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule().get(),
                            var_ty,
                            true,
                            initializer
                    );
                    scope.push(it->var_name, var, true);
                }
            }
            else {
                if (it->array) {
                    Constant *initializer = (Constant *) new ConstantArrayInitializer(it->var_name, dimension, init);
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule().get(),
                            var_ty,
                            false,
                            initializer
                    );
                    scope.push(it->var_name, var, true, dimension, init);
                } else {
                    auto *initializer = (Constant *) new ConstantInt(init[0]);
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule().get(),
                            var_ty,
                            false,
                            initializer
                    );
                    scope.push(it->var_name, var, true);
                }
            }
        } else {
            auto var = builder->create_alloca(var_ty);
            scope.push(it->var_name, var, node.isConst(), dimension, init);
            if (it->array && it->has_initial) {

            }
        }


        /** TODO: Finish initializer

        if (node.num == nullptr) {
            if (scope.in_global()) {
                auto initializer = ConstantZero::get(var_type, module.get());
                auto var = GlobalVariable::create
                        (
                                node.id,
                                module.get(),
                                var_type,
                                false,
                                initializer);
                scope.push(node.id, var);
            } else {
                auto var = builder->create_alloca(var_type);
                scope.push(node.id, var);
            }
        } else {
            auto *array_type = ArrayType::get(var_type, node.num->i_val);
            if (scope.in_global()) {
                auto initializer = ConstantZero::get(array_type, module.get());
                auto var = GlobalVariable::create
                        (
                                node.id,
                                module.get(),
                                array_type,
                                false,
                                initializer);
                scope.push(node.id, var);
            } else {
                auto var = builder->create_alloca(array_type);
                scope.push(node.id, var);
            }
        }
         **/
    }

}
void BZBuilder::visit(ASTFuncDecl &node){
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

void BZBuilder::visit(ASTParam &node){
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

void BZBuilder::visit(ASTAssignStmt &node) {
    node.getLeftValue()->accept(*this);
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
