#ifndef BZCOMPILER_CONSTANT
#define BZCOMPILER_CONSTANT
#include "Type.h"
#include "Module.h"
#include <utility>
#include <vector>

class Constant {
public:
    Constant() = default;;
private:
};

class ConstantInt : public Constant {
public:
    explicit ConstantInt(int val);

    static int getValue(ConstantInt *const_val);

    int getValue() const;

    void setValue(int val);

    static ConstantInt *get(int val);

private:
    int value_;
};

class ConstantArray : public Constant {
public:
    explicit ConstantArray(const std::vector<Constant *>& val);


    unsigned get_elements_num();

    static ConstantArray *get(const std::vector<Constant *> &val);

private:
    std::vector<Constant *> const_array;
};

class ConstantArrayInitializer : public Constant {
private:
    const std::vector<int> _dim;
    const std::vector<int> _initial;
    const std::string _name;
public:
    ConstantArrayInitializer(std::string initializerName, const std::vector<int> &dim, const std::vector<int> &initial) :
            _dim(dim), _initial(initial), _name(std::move(initializerName)){
        int num_val = 1;
        for(int d: dim) {
            num_val *= d;
        }
        assert(num_val == initial.size() && "Initializer got mismatch dimension vs number to be init.");
    }

    std::string print() {
        std::vector<std::string> st;
        std::vector<std::string> ost;
        std::string prefix = "i32";
        for (int i : _initial) {
            ost.emplace_back("i32 " + std::to_string(i));
        }
        int num_group = _initial.size();
        for (int i = (int)_dim.size() - 1; i >= 0; --i) {
            prefix = '[' + std::to_string(_dim[i]) + " x " + prefix + ']';
            num_group /= _dim[i];
            int offset = 0;
            for (int gp = 0; gp < num_group; ++gp) {
                std::string builder = "[";
                for (int j = 0; j < _dim[i]; ++j) {
                    builder += ost[offset + j];
                    builder += ',';
                }
                offset += _dim[i];
                builder.pop_back();
                builder += ']';
                builder = prefix + builder;
                st.push_back(builder);
            }
            ost = st;
            st.clear();
        }
        return "@" + _name + " = private unnamed_addr constant " + ost[0];
    }
};

class ConstantIntInitializer {

};

#endif