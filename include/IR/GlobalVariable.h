#include <Type.h>
#include <string>
#include <Module.h>
#include <Constant.h>
class GlobalVariable{
    public:
        GlobalVariable(std::string name, Module *m, Type *ty, bool is_const, Constant *init = nullptr);
        Constant *get_init();
        static GlobalVariable* create(std::string name, Module *m, Type *type, bool is_const, Constant *init);
    private:
        bool is_const_;
        Constant* init_val_;
};