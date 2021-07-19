//
// Created by Misakihanayo on 2021/7/17.
//

#include "bzcompiler_builder.hpp"
#include <stack>

#define CONST(num) ConstantInt::get(num, &*module)

#define _IRBUILDER_ERROR_(str)                                                  \
{                                                                               \
    std::cerr << "Error in IRbuilder-> " << str << std::endl;                   \
    std::abort();                                                               \
}


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
std::vector<Argument *> cur_fun_param;
int cur_fun_param_num = 0;
bool pre_enter_scope = false;
//
std::vector<Value *> array_init;
//
// std::vector<BasicBlock*> iter_expr,iter_cont;
enum CurBaseListType { WHILE_COND, WHILE_BODY, IF_COND, IF_THEN, IF_ELSE };

std::vector<CurBaseListType> BL_types;

// std::vector<BaseBlock *> base_layer;
int tmp_int = 0;
bool use_int = false;
bool in_global_init = false;

int label_idx = 0;

std::string getNewLabel() {
    return "lb" + std::to_string(label_idx++);
}

Function * currentfunc;
BasicBlock * orTrueExit;

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
    tmp_val = CONST(node.getValue());
    //tmp_int = node.getValue();
    //use_int = true;
}
void BZBuilder::visit(ASTUnaryOp &node)
{/*
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
*/
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
        val = builder->create_icmp_eq(val, CONST(0));
        break;
    }
    tmp_val = val;
}

void BZBuilder::visit(ASTMulOp &node)
{
    if(node.isUnaryExp())
        node.getOperand1()->accept(*this);
    else
    {
        /*
        if(use_int){
            node.getOperand1()->accept(*this);
            auto l_val = tmp_int;
            node.getOperand2()->accept(*this);
            auto r_val = tmp_int;
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
*/
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->get_type()->is_int1_type()) {
            l_val = builder->create_zext(l_val, TyInt32);
        }

        if (r_val->get_type()->is_int1_type()) {
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
    if(node.isUnaryExp())
        node.getOperand1()->accept(*this);
    else
    {/*
        if(use_int){
            node.getOperand1()->accept(*this);
            auto l_val = tmp_int;
            node.getOperand2()->accept(*this);
            auto r_val = tmp_int;
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
*/
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;
        
        if (l_val->get_type()->is_int1_type()) {
            l_val = builder->create_zext(l_val, TyInt32);
        }

            if (r_val->get_type()->is_int1_type()) {
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
    if (node.isUnaryExp()) {
        node.getOperand1()->accept(*this);
        node.setTrueList(node.getOperand1()->getTrueList());
        node.setFalseList(node.getOperand1()->getFalseList());
    }
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->get_type()->is_int1_type())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->get_type()->is_int1_type()) {
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
        // 适配短路运算
        BasicBlock *t_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
        BasicBlock *f_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
        builder->create_cond_br(tmp_val, t_bb, f_bb);
        node.pushTrue(t_bb);
        node.pushFalse(f_bb);
    }

}
void BZBuilder::visit(ASTEqOp &node)
{
    if (node.isUnaryExp()){
        node.getOperand1()->accept(*this);
        node.setTrueList(node.getOperand1()->getTrueList());
        node.setFalseList(node.getOperand1()->getFalseList());
    }
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;

        if (l_val->get_type()->is_int1_type())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->get_type()->is_int1_type()) {
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
        // 适配短路运算
        BasicBlock *t_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
        BasicBlock *f_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
        builder->create_cond_br(tmp_val, t_bb, f_bb);
        node.pushTrue(t_bb);
        node.pushFalse(f_bb);
    }
}

// FIXME: 短路运算符
void BZBuilder::visit(ASTAndOp &node)
{
    if (node.isUnaryExp()) {
        node.getOperand1()->accept(*this);
        if (node.getOperand1()->getTrueList().empty() && node.getOperand1()->getFalseList().empty()) {
            auto cod = builder->create_icmp_ne(CONST(0), tmp_val);
            BasicBlock *t_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
            BasicBlock *f_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
            builder->create_cond_br(cod, t_bb, f_bb);
            node.pushTrue(t_bb);
            node.pushFalse(f_bb);
        } else {
            node.setTrueList(node.getOperand1()->getTrueList());
            node.setFalseList(node.getOperand1()->getFalseList());
        }
    }
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        /// 标号回填 B -> B1 and <b>M<\b> B2
        auto new_b = BasicBlock::create(getModule(), getNewLabel(), currentfunc);  // M.quad
        builder->set_insert_point(new_b);
        /// 标号回填阶段1结束
        node.getOperand2()->accept(*this);
        if (node.getOperand2()->getTrueList().empty() && node.getOperand2()->getFalseList().empty()) {
            auto cod = builder->create_icmp_ne(CONST(0), tmp_val);
            BasicBlock *t_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
            BasicBlock *f_bb = BasicBlock::create(getModule(), getNewLabel(), currentfunc, true);
            builder->create_cond_br(cod, t_bb, f_bb);
            node.getOperand2()->pushTrue(t_bb);
            node.getOperand2()->pushFalse(f_bb);
        } else {
            node.setTrueList(node.getOperand1()->getTrueList());
            node.setFalseList(node.getOperand1()->getFalseList());
        }

        /// 标号回填 B -> B1 and M <b>B2<\b>
        // Back patch B1.true_list with M.quad
        for (auto bb: node.getOperand1()->getTrueList()) {
            bb->replace_all_use_with(new_b);
        }
        // Merge Lists
        node.setTrueList(node.getOperand2()->getTrueList());
        node.setFalseList(node.getOperand1()->getFalseList());
        node.unionFalseList(node.getOperand2()->getFalseList());
        /// 标号回填阶段2结束
        /* TODO: 或许不需要了？
        auto r_val = tmp_val;

        if (l_val->get_type()->is_int1_type())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->get_type()->is_int1_type()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }
        // FIXME: LLVM IR 中并没有计算 and/or 的指令，需要使用基本块来实现
        tmp_val = builder->create_iand(l_val, r_val);
         */
    }
}
// FIXME: 短路运算符
void BZBuilder::visit(ASTOrOp &node)
{
    if (node.isUnaryExp())
    {
        node.getOperand1()->accept(*this);
        node.setTrueList(node.getOperand1()->getTrueList());
        node.setFalseList(node.getOperand1()->getFalseList());
    }
    else {
        node.getOperand1()->accept(*this);
        auto l_val = tmp_val;
        /// 标号回填 B -> B1 or <b>M<\b> B2
        auto new_b = BasicBlock::create(getModule(), getNewLabel(), currentfunc);  // M.quad
        builder->set_insert_point(new_b);
        /// 标号回填阶段1结束
        node.getOperand2()->accept(*this);
        auto r_val = tmp_val;
        /// 标号回填 B -> B1 or M <b>B2<\b>
        // Back patch B1.true_list with M.quad
        for (auto bb: node.getOperand1()->getFalseList()) {
            bb->replace_all_use_with(new_b);
        }
        // Merge Lists
        node.setTrueList(node.getOperand1()->getTrueList());
        node.unionTrueList(node.getOperand2()->getTrueList());
        node.setFalseList(node.getOperand2()->getFalseList());
        /// 标号回填阶段2结束
        /*
        if (l_val->get_type()->is_int1_type())
            l_val = builder->create_zext(l_val, TyInt32);

        if (r_val->get_type()->is_int1_type()) {
            r_val = builder->create_zext(r_val, TyInt32);
        }

        tmp_val = builder->create_ior(l_val, r_val);
        if (tmp_val->get_type()->is_int32_type()) {
            tmp_val = builder->create_icmp_ne(tmp_val, CONST(0));
        }
        */
    }
}
void BZBuilder::visit(ASTLVal &node)
{
    std::string var_name = node.getVarName();
    std::vector<ASTAddOp *> pointer_exp= node.getPointerExpression();
    auto var = scope.find(var_name);
    if (var->get_type()->is_integer_type()) {
        tmp_val = var;
        return;
    }
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    if (pointer_exp.empty()) {
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
            scope.find_params(node.getVarName(), array_params);
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
    for (int i = 0; i < params.size(); i++) {
        auto arg = params[i];
        auto arg_type =
                dynamic_cast<Function *>(func_name)->get_function_type()->get_param_type(i);
        if (arg_type->is_integer_type())
            require_address = false;
        else
            require_address = true;
        arg->accept(*this);
        require_address = false;
        args.push_back(tmp_val);
    }
    tmp_val = builder->create_call(dynamic_cast<Function *>(func_name), args);
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
        default:
            assert(0 && "Not a compile-time calculable expression.");
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
            return {depth, offset[offset.size() + 1 - delta]};
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


std::tuple<int, int>
BZBuilder::InitialValueWalker(ASTVarDecl::ASTArrayList *l, const std::vector<int> &offset, int depth,
                                   std::vector<Value *> &init_values, Module *m) {
    if (l->isEmpty) {
        for (int i = 0; i < offset[offset.size() - 1]; ++i) {
            init_values.push_back(ConstantZero::get(Type::get_int32_type(m), m));
        }
        return std::make_tuple(depth + 1, offset[offset.size() - 1]);
    }
    if (l->isArray) {
        int max_depth = 0;
        int filled = 0;
        for (auto arr: l->list) {
            int _set, _depth;
            std::tie(_depth, _set) = InitialValueWalker(arr, offset, depth + 1, init_values, m);
            max_depth = std::max(max_depth, _depth);
            filled += _set;
        }
        int delta = max_depth - depth;
        if (delta < 2) {
            return {depth, offset[offset.size() + 1 - delta]};
        } else {
            int tb_fill = offset[offset.size() + 1 - delta];
            for (int i = 0; i < tb_fill - filled; ++i) {
                init_values.push_back(ConstantZero::get(Type::get_int32_type(m), m));
            }
            return {depth, tb_fill};
        }
    } else {
        l->value->accept(*this);
        init_values.push_back(tmp_val);
        return std::make_tuple(depth, 1);
    }
}

void BZBuilder::InitialValueBuilder(const std::vector<int> &dim, const std::vector<Value *> &val, Instruction *gep, int &offset, int depth) {
    for (int i = 0; i < dim[depth]; ++i) {
        auto g_i = builder->create_gep(gep, {ConstantInt::get(i, getModule())});
        if (depth != dim.size() - 1) {
            InitialValueBuilder(dim, val, g_i, offset, depth + 1);
        } else {
            builder->create_store(g_i, val[offset++]);
        }
    }
}

void BZBuilder::visit(ASTVarDecl &node) {
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

        Type *var_ty = Type::get_int32_type(getModule());
        for (int i = (int)dimension.size() - 1; i >= 0; --i) {
            var_ty = new ArrayType(var_ty, dimension[i]);
        }


        if (scope.in_global()) {
            if (node.isConst()) {
                if (it->array) {
                    // 全局静态数组
                    Constant *initializer = ConstantArray::get(dynamic_cast<ArrayType *>(var_ty), ConstantArray::IntegerList2Constant(dimension, init, getModule()));
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule(),
                            var_ty,
                            true,
                            initializer
                    );
                    scope.push(it->var_name, var, true, dimension, init);
                } else {
                    // 全局静态常量
                    auto *initializer = ConstantInt::get(init[0], getModule());
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule(),
                            var_ty,
                            true,
                            initializer
                    );
                    scope.push(it->var_name, var, true);
                }
            }
            else {
                if (it->array) {
                    // 全局动态数组
                    Constant *initializer = ConstantArray::get(dynamic_cast<ArrayType *>(var_ty), ConstantArray::IntegerList2Constant(dimension, init, getModule()));
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule(),
                            var_ty,
                            false,
                            initializer
                    );
                    scope.push(it->var_name, var, true, dimension, init);
                } else {
                    // 全局动态变量
                    auto *initializer = ConstantInt::get(init[0], getModule());
                    auto var = GlobalVariable::create(
                            it->var_name,
                            getModule(),
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
            if (it->has_initial) {
                if(it->array) {
                    std::vector<Value *> _init;
                    InitialValueWalker(it->initial_value[0], dimension, 0, _init, getModule());
                    int offset;
                    InitialValueBuilder(dimension, _init, var, offset, 0);
                } else {
                    it->initial_value[0]->value->accept(*this);
                    if (tmp_val->get_type()->is_pointer_type()) {
                        tmp_val = builder->create_load(tmp_val);
                    }
                    builder->create_store(tmp_val, var);
                }
            }
        }


        /** TODO: Finish initializer

        if (node.num == nullptr) {
            if (scope.in_global()) {
                auto initializer = ConstantZero::get(var_type, module);
                auto var = GlobalVariable::create
                        (
                                node.id,
                                module,
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
                auto initializer = ConstantZero::get(array_type, module);
                auto var = GlobalVariable::create
                        (
                                node.id,
                                module,
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
        fun_ret_type = Type::get_int32_type(getModule());
    }
    else{
        fun_ret_type = Type::get_void_type(getModule());
    }
    auto params = node.getParams();
    std::vector<Type *> args;
    std::vector<Value *> fun_args;
    for(auto param : params){
        if(param->isArray()){
            args.push_back(Type::get_int32_ptr_type(getModule()));
        }
        else{
            args.push_back(Type::get_int32_type(getModule()));
        }
    }
    auto fun_type = FunctionType::get(fun_ret_type, args);
    auto fun =  Function::create(fun_type, node.getFunctionName(), getModule());
    auto bb = BasicBlock::create(getModule(), "entry", fun);
    builder->set_insert_point(bb);
    scope.push(node.getFunctionName(), fun);
    scope.enter();
    cur_fun_param.clear();
    currentfunc = fun;
    cur_fun_param_num = 0;
    auto fun_param = fun->get_args();
    for(auto & it : fun_param) {
        cur_fun_param.push_back(it);
    }
    for(auto param: params){
        param->accept(*this);
        cur_fun_param_num ++;
    }
    auto block = node.getStmtBlock();
    block->accept(*this);
    scope.exit();
}

void BZBuilder::visit(ASTParam &node){
    if(node.isArray()){
        auto array_alloca = builder->create_alloca(Type::get_int32_ptr_type(getModule()));
        // FIXME: Value是基类，不能这样进行创建
        auto param = cur_fun_param[cur_fun_param_num];
        // Value* arg = new Value(Type::get_int32_ptr_type(getModule()), node.getParamName())
        builder->create_store(param, array_alloca);
        std::vector<Value *> array_params;
        array_params.push_back(ConstantInt::get(0, getModule()));
        for (auto array_param : node.getArrayList()) {
            array_param->accept(*this);
            array_params.push_back(tmp_val);
        }
        // FIXME: 未知定义。注意Array可能是高维数组
        scope.push(node.getParamName(), array_alloca);
        scope.push_params(node.getParamName(), array_alloca, array_params);
    }
    else{
        auto alloca = builder->create_alloca(Type::get_int32_type(getModule()));
        auto params = node.getArrayList();
        // // FIXME: 同上
        // Value* arg = new Value(Type::get_int32_type(getModule()), node.getParamName())
        builder->create_store(cur_fun_param[cur_fun_param_num], alloca);
        scope.push(node.getParamName(), alloca);
    }
}

void BZBuilder::visit(ASTAssignStmt &node) {
    node.getLeftValue()->accept(*this);
    auto assign_addr=tmp_val;
    node.getExpression()->accept(*this);
    auto assign_value=tmp_val;
    // FIXME: API使用错误
    if (assign_addr->get_type()->is_pointer_type()) {
        builder->create_store(assign_value, assign_addr);
        // assign_value = assign_addr->create_load(assign_value);
    }
}
void BZBuilder::visit(ASTExpressionStmt &node) {
    if (node.isValidExpression())
        node.getExpression()->accept(*this);
}
void BZBuilder::visit(ASTIfStmt &node) {
    auto tmp=builder->get_insert_block();
    if (node.hasElseStatement()){
        auto trueBB=BasicBlock::create(module,getNewLabel(),currentfunc);
        auto falseBB=BasicBlock::create(module,getNewLabel(),currentfunc);
        auto exitBB=BasicBlock::create(module,getNewLabel(),currentfunc);
        orTrueExit=trueBB;
        builder->set_insert_point(tmp);
        node.getCondition()->accept(*this);
        // builder->create_cond_br(ret,trueBB,falseBB);
        builder->set_insert_point(trueBB);
        node.getTrueStatement()->accept(*this);

        /// 标号回填
        // 两个list都为空，说明是ASTAdd指令传上来的
        if (node.getCondition()->getTrueList().empty() && node.getCondition()->getFalseList().empty()) {
            builder->set_insert_point(tmp);
            auto cod = builder->create_icmp_eq(CONST(0), tmp_val);
            builder->create_cond_br(cod, trueBB, falseBB);
        }
        // back patch B.true_list with M1.quad
        for (auto bb: node.getCondition()->getTrueList()) {
            bb->replace_all_use_with(trueBB);
        }
        // back patch B.false_list with M2.quad
        for (auto bb: node.getCondition()->getFalseList()) {
            bb->replace_all_use_with(falseBB);
        }
        /// 标号回填结束

        bool isReturned=true;
        if (builder->get_insert_block()->get_terminator()==nullptr){
            builder->create_br(exitBB);
            isReturned=false;
        }
        builder->set_insert_point(falseBB);
        node.getFalseStatement()->accept(*this);
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
        auto trueBB=BasicBlock::create(module,getNewLabel(),currentfunc);  // M.quad
        auto exitBB=BasicBlock::create(module,getNewLabel(),currentfunc);  // NextList回填目标
        orTrueExit=trueBB;
        builder->set_insert_point(tmp);
        node.getCondition()->accept(*this);
        // builder->create_cond_br(ret,trueBB,exitBB);
        /// 标号回填
        // back patch B.true_list with M.quad
        for (auto bb: node.getCondition()->getTrueList()) {
            bb->replace_all_use_with(trueBB);
        }
        // back patch B.false_list (node.next_list) with exit.quad
        for (auto bb: node.getCondition()->getFalseList()) {
            bb->replace_all_use_with(exitBB);
        }
        /// 标号回填结束
        builder->set_insert_point(trueBB);
        node.getTrueStatement()->accept(*this);
        bool isReturned=true;
        if (builder->get_insert_block()->get_terminator()==nullptr){
            builder->create_br(exitBB);
            isReturned=false;
        }
        builder->set_insert_point(exitBB);
    }
    orTrueExit=nullptr;
}
void BZBuilder::visit(ASTWhileStmt &node) {
    auto tmp=builder->get_insert_block();
    auto judgebb=BasicBlock::create(module,getNewLabel(),currentfunc);
    auto iteratebb=BasicBlock::create(module,getNewLabel(),currentfunc);
    auto exitbb=BasicBlock::create(module,getNewLabel(),currentfunc);
    builder->set_insert_point(tmp);
    builder->create_br(judgebb);
    builder->set_insert_point(judgebb);
    orTrueExit=iteratebb;
    node.getCondition()->accept(*this);
    builder->create_cond_br(tmp_val,iteratebb,exitbb);

    builder->set_insert_point(iteratebb);
    curIteration.push(iteratebb);
    curIterationExit.push(exitbb);
    curIterationJudge.push(judgebb);
    node.getWhileBodyStatement()->accept(*this);
    builder->create_br(judgebb);
    builder->set_insert_point(exitbb);
    curIteration.pop();
    curIterationExit.pop();
    curIterationJudge.pop();
    /// 标号回填
    // 两个list都为空，说明是ASTAdd指令传上来的
    if (node.getCondition()->getTrueList().empty() && node.getCondition()->getFalseList().empty()) {
        builder->set_insert_point(tmp);
        auto cod = builder->create_icmp_eq(CONST(0), tmp_val);
        builder->create_cond_br(cod, iteratebb, exitbb);
    }
    // back patch B.true_list with M.quad
    for (auto bb: node.getCondition()->getTrueList()) {
        bb->replace_all_use_with(iteratebb);
    }
    // back patch B.false_list (node.next_list) with exit.quad
    for (auto bb: node.getCondition()->getFalseList()) {
        bb->replace_all_use_with(exitbb);
    }
    /// 标号回填结束
}
void BZBuilder::visit(ASTBreakStmt &node) {
    builder->create_br(curIterationExit.top());
}
void BZBuilder::visit(ASTContinueStmt &node) {
    builder->create_br(curIterationJudge.top());
}

void BZBuilder::visit(ASTReturnStmt &node) {
    if (node.hasReturnValue()) {
        node.getRetExpression()->accept(*this);
        builder->create_ret(tmp_val);
    } else {
        builder->create_void_ret();
    }
}
void BZBuilder::visit(ASTBlock &node) {
    scope.enter();
    for (auto exp: node.getStatements()) {
        exp->accept(*this);
    }
    scope.exit();
}
