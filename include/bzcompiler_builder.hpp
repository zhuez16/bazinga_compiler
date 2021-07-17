#ifndef _CMINUSF_BUILDER_HPP_
#define _CMINUSF_BUILDER_HPP_
#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"
#include "ast.h"
#include <map>



class Scope {
public:
    // enter a new scope
    void enter() {
        inner.push_back({});
    }

    // exit a scope
    void exit() {
        inner.pop_back();
    }

    bool in_global() {
        return inner.size() == 1;
    }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(std::string name, Value *val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    bool push_params(std::string name, Value *val, std::vector<Value *> params) {
        auto result = array_param[array_param.size() - 1].insert({name, params});
        return result.second;
    }
    
    Value* find(std::string name) {
        for (auto s = inner.rbegin(); s!= inner.rend();s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }

        return nullptr;
    }

private:
    std::vector<std::map<std::string, Value *>> inner;
};


class bzBuilder: public ASTvisitor {
public:
    bzBuilder() {
        module = std::unique_ptr<Module>(new Module("bzcomp code"));
        builder = new IRBuilder(nullptr, module.get());
        auto TyVoid = Type::get_void_type(module.get());
        auto TyInt32 = Type::get_int32_type(module.get());
    }

    std::unique_ptr<Module> getModule() {
        return std::move(module);
    }
private:
    virtual void visit(ASTInstruction &);
    virtual void visit(ASTProgram &) override final;
    virtual void visit(ASTConstant &) override final;
    virtual void visit(ASTUnaryOp &) override final;
    virtual void visit(ASTMulOp &) override final;
    virtual void visit(ASTAddOp &) override final;
    virtual void visit(ASTRelOp &) override final;
    virtual void visit(ASTEqOp &) override final;
    virtual void visit(ASTAndOp &) override final;
    virtual void visit(ASTOrOp &) override final;
    virtual void visit(ASTLVal &) override final;
    virtual void visit(ASTFuncCall &) override final;
    virtual void visit(ASTStatement &) override final;
    virtual void visit(ASTDecl &) override final;
    virtual void visit(ASTVarDecl &) override final;
    virtual void visit(ASTAssignStmt &) override final;
    virtual void visit(ASTExpressionStmt &) override final;
    virtual void visit(ASTIfStmt &) override final;
    virtual void visit(ASTWhileStmt &) override final;
    virtual void visit(ASTBreakStmt &) override final;
    virtual void visit(ASTContinueStmt &) override final;

    IRBuilder *builder;
    Scope scope;
    std::unique_ptr<Module> module;
};
#endif
