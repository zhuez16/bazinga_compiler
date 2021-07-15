#include <Type.h>
#include <Module.h>
#include <vector>
class Constant{
    public:
        Constant(){};
    private:
};

class ConstantInt : public Constant{
    public:
        ConstantInt(int val);
        static int getValue(ConstantInt *const_val);
        int getValue();
        void setValue(int val);
        static ConstantInt *get(int val);
    private:
        int value_;
};

class ConstantArray : public Constant{
    public:
        ConstantArray(std::vector<Constant *> val);
        unsigned get_elements_num();
        static ConstantArray* get(const std::vector<Constant *> &val);
    private:
        std::vector<Constant *> const_array;
};