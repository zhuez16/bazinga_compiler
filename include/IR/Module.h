#ifndef SYSYC_MODULE_H
#define SYSYC_MODULE_H

#include "Type.h"
#include "Instruction.h"
#include <vector>
#include <map>
#include <list>
#include "Function.h"
#include <string>
#include "GlobalVariable.h"
class Module{
    private:
        std::vector<Function* > function_lists;
        std::list<GlobalVariable *> global_vars;
        std::map<Instruction*, std::string> instr_name;
        Type* VoidType;
        IntergerType* IntType;
        IntergerType* BoolType;
        Type* LabelType;
        PointerType* Int32PtrType;
    public:
        Module(std::string name);
        Type* get_Void_type();
        // Type* get_Int_type();
        IntegerType* get_Int1_type();
        IntegerType* get_Int32_type();
        Type* get_Label_type();
        PointerType* get_Int32Ptr_type();
        
        void add_function(Function* func);
        void add_global_var(GlobalVariable* var);
        std::list<GlobalVariable *>get_global_variable();
        std::string get_instr_name(Instruction::OpID oprand);
        void set_print_name();
<<<<<<< HEAD
    
};
=======
    private:
        std::vector<Function* > function_lists;
        std::list<GlobalVariable *> global_vars;
        std::map<Instruction*, std::string> instr_name;
        Type* VoidType;
        Type* IntType;
        Type* BoolType;
};

#endif
>>>>>>> bbd28796f21caf2415b273db404e77e16943a54f
