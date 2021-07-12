#include <vector>
#include <Type.c++>
#include <Module.c++>
// class definition

class Constant{
    public:
        Constant(Type* ty){

        }
    private:
};

class ConstantInt : public Constant{
    public:
        static int getValue(ConstantInt *const_val);
        int getValue();
        void setValue(int val);
        static ConstantInt *get(int val, Module *m);
    private:
        int value_;
};

class ConstantArray : public Constant{
    public:
        unsigned get_elements_num();
        static ConstantArray* get(ArrayTpye *ty, const std::vector<Constant *> &val);
    private:
        std::vector<Constant *> const_array;
};

// ConstantInt ans COnstantArray API

int ConstantInt::getValue(ConstantInt *const_val){
    return const_val->value_;
}

int ConstantInt::getValue(){
    return this->value_;
}

void ConstantInt::setValue(int val){
    this->value_ = val;
}

ConstantInt *get(int val, Module *m){
    return new 
}