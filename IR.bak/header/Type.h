#ifndef SYSYC_TYPE_H
#define SYSYC_TYPE_H

#include <iostream>
#include <vector>

class Module;

class IntegerType;

class FunctionType;

class ArrayType;

class PointerType;

class Type {
public:
    enum TypeID {
        void_type,     // Void
        Label_type,    // Labels, e.g., BasicBlock
        Integer32_type,  // Integers, include 32 bits and 1 bit
        Integer1_type,
        Function_type, // Functions
        Array_type,    // Arrays
        Pointer_type,  // Pointer
    };

    explicit Type(TypeID tid);

    ~Type() = default;

    TypeID get_TypeID() const { return type_id; }

    bool is_Void_type() const { return get_TypeID() == void_type; }

    bool is_Label_type() const { return get_TypeID() == Label_type; }

    bool is_Integer_type() const { return get_TypeID() == Integer32_type || get_TypeID() == Integer1_type; }

    bool is_Function_type() const { return get_TypeID() == Function_type; }

    bool is_Array_type() const { return get_TypeID() == Array_type; }

    bool is_Pointer_type() const { return get_TypeID() == Pointer_type; }

    bool is_Int1() const { return get_TypeID() == Integer1_type; };

    bool is_Int32() const { return get_TypeID() == Integer32_type; };

    static Type *get_Void_type(Module *m);

    static Type *get_Label_type(Module *m);

    static IntegerType *get_Int1_type(Module *m);

    static IntegerType *get_Int32_type(Module *m);

    static PointerType *get_Int32Ptr_type(Module *m);

    Type *get_PtrElement_type();

    void print();

    std::string CommentPrint();

    int get_size(bool extended = true);

    //判断与类型是否相等
    bool equal(Type x) {
        if (this->type_id != x.type_id) {
            return false;
        } else if (this->is_Pointer_type()) {
            return this->get_PtrElement_type()->equal(*x.get_PtrElement_type());
        } else {
            return true;
        }
    }

private:
    TypeID type_id;
};

class IntegerType : public Type {
public:
    explicit IntegerType(unsigned num_bits);

    static IntegerType *get(unsigned num_bits);

    unsigned get_num_bits_();

private:
    unsigned num_bits_;
};

class FunctionType : public Type {
public:
    FunctionType(Type *ret, std::vector<Type *> args);

    static bool check_return_type(Type *type);

    static bool check_arguement_type(Type *type);

    static FunctionType *get(Type *ret, std::vector<Type *> args);

    unsigned get_args_num() const;

    Type *get_args_type(unsigned i) const;

    Type *get_return_type() const;

private:
    Type *ret_;
    std::vector<Type *> args_;
};

class ArrayType : public Type {
public:
    ArrayType(Type *contained, unsigned num_elements);

    static bool check_element_type(Type *type);

    static ArrayType *get(Type *contained, unsigned num_elements);

    Type *get_element_type() const { return contained_; }

    unsigned get_elements_num() const { return num_elements_; }

    std::vector<unsigned> get_Dims() const;

private:
    Type *contained_;       // The element type of the array.
    unsigned num_elements_; // Number of elements in the array.
};

class PointerType : public Type {
public:
    PointerType(Type *contained);

    Type *get_element_type() const { return contained_; }

    static PointerType *get(Type *contained);

private:
    Type *contained_; // The element type of the ptr.
};

#endif // SYSYC_TYPE_H
