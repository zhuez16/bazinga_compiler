#ifndef BZCOMPILER_GLB_VAR
#define BZCOMPILER_GLB_VAR

#include "IR/Type.h"
#include <string>
#include "IR/Module.h"
#include "IR/Constant.h"

class Constant;
class ConstantInt;
class ConstantArrayInitializer;

class GlobalVariable {
public:
    GlobalVariable(const std::string& name, Module *m, Type *ty, bool is_const, Constant *init = nullptr);

    Constant *get_init();

    static GlobalVariable *create(const std::string& name, Module *m, Type *type, bool is_const, Constant *init);

private:
    bool is_const_{};
    Constant *init_val_{};
};

#endif