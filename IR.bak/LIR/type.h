//
// Created by 顾超 on 2021/7/17.
//

#ifndef BAZINGA_COMPILER_TYPE_H
#define BAZINGA_COMPILER_TYPE_H

#include <vector>
using namespace std;

class Type {
public:
    enum LIR_TYPE {
        T_INTEGER,
        T_FUNCTION,
        T_ARRAY,
        T_VOID,
        T_LABEL
    };
private:
    LIR_TYPE _type;

public:
    LIR_TYPE getTypeID() const { return _type; }
};


class FunctionType : public Type {
private:
    vector<Type *> _params_t;
    Type *_return_t;
    FunctionType(Type *returnType, const vector<Type *> &params) {
        assert(isValidReturnType(returnType) && "Function return type is invalid.");
        for (Type *t: params) {
            assert(isValidParamType(t) && "Function param type is invalid.");
        }
        _params_t = params;
        _return_t = returnType;
    }
public:
    static FunctionType* get(Type *returnType, vector<Type *> params = {}) {return new FunctionType(returnType, params);}
    unsigned int getNumParams() const { return _params_t.size(); }
    Type *getParamType(unsigned int i) { return _params_t[i]; }
    Type *getReturnType() { return _return_t; }
    vector<Type *> getParams() { return _params_t; }
    inline bool isValidReturnType(const Type *tp) { return tp->getTypeID() == T_VOID || tp->getTypeID() == T_INTEGER; }
    inline bool isValidParamType(const Type *tp) { return tp->getTypeID() == T_INTEGER || tp->getTypeID() == T_ARRAY; }
};

#endif //BAZINGA_COMPILER_TYPE_H
