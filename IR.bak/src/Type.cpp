#include <Type.h>
#include <Module.h>
#include <cassert>
//#include "ReturnVal.h"

Type::Type(TypeID tid) { type_id = tid; }

bool Type::is_Int1() {
  if (get_TypeID() == Integer_type)
    if (static_cast<IntegerType *>(this)->get_num_bits_() == 1)
      return true;
  return false;
}

bool Type::is_Int32() {
  if (get_TypeID() == Integer_type)
    if (static_cast<IntegerType *>(this)->get_num_bits_() == 32) 
      return true;
  return false;
}

Type *Type::get_Void_type(Module *m) { return m->get_Void_type(); }

Type *Type::get_Label_type(Module *m) { return m->get_Label_type(); }

IntegerType *Type::get_Int1_type(Module *m) { return m->get_Int1_type(); }

IntegerType *Type::get_Int32_type(Module *m) { return m->get_Int32_type(); }

PointerType *Type::get_Int32Ptr_type(Module *m) { return m->get_Int32Ptr_type(); }

Type *Type::get_PtrElement_type() {
  if (this->is_Pointer_type())
    return static_cast<PointerType *>(this)->get_element_type();
  else
    return nullptr;
}

int Type::get_size(bool extended) {
  if (this->is_Integer_type()) {
    auto bits = static_cast<IntegerType *>(this)->get_num_bits_();
    if(bits==1)
        return 1;
    else
        return 4;
  }
  if (this->is_Array_type()) {
    auto size = static_cast<ArrayType *>(this)->get_element_type()->get_size();
    auto num = static_cast<ArrayType *>(this)->get_elements_num();
    return size * num;
  }
  if (this->is_Pointer_type()) {
    if (extended && this->get_PtrElement_type()->is_Array_type()) {
      return this->get_PtrElement_type()->get_size();
    } else {
      return 4;
    }
  }
  return 0;
}

void Type::print() {
  switch (type_id) {
  case Label_type:
    std::cerr << "<label>";
    break;

  case Integer_type:
    if (static_cast<IntegerType *>(this)->get_num_bits_() == 1) {
      std::cerr << "i1";
    } else {
      std::cerr << "i32";
    }
    break;

  case Array_type:
    std::cerr << "[ " << static_cast<ArrayType *>(this)->get_elements_num()
              << " x ";
    static_cast<ArrayType *>(this)->get_element_type()->print();
    std::cerr << "]";
    break;

  case Pointer_type:
    get_PtrElement_type()->print();
    std::cerr << "*";
    break;

  default:
    break;
  }
  return;
}

std::string Type::CommentPrint() {
  std::string typeString;
  switch (type_id) {
  case Label_type:
    typeString += "<label>";
    break;

  case Integer_type:
    if (static_cast<IntegerType *>(this)->get_num_bits_() == 1) {
      typeString += "i1";
    } else {
      typeString += "i32";
    }
    break;

  case Array_type:
    typeString += "[ ";
    typeString +=
        std::to_string(static_cast<ArrayType *>(this)->get_elements_num()) +
        " x ";
    typeString +=
        static_cast<ArrayType *>(this)->get_element_type()->CommentPrint();
    typeString += "]";
    break;

  case Pointer_type:
    typeString += get_PtrElement_type()->CommentPrint();
    typeString += "*";
    break;

  default:
    break;
  }
  return typeString;
}

IntegerType::IntegerType(unsigned num_bits)
    : Type(Type::Integer_type), num_bits_(num_bits) {}

IntegerType *IntegerType::get(unsigned num_bits) {
  return new IntegerType(num_bits);
}

unsigned IntegerType::get_num_bits_() { return num_bits_; }

FunctionType::FunctionType(Type *ret, std::vector<Type *> args)
    : Type(Type::Function_type) {
//exit_ifnot ?
  //exit_ifnot(_InvalidRetVal_Constructor_FunctionType,check_return_type(ret) && "Invalid return type for function!");
  ret_ = ret;

  for (auto p : args) {
    //exit_ifnot(_InvalidArgType_Constructor_FunctionType,check_arguement_type(p) &&"Not a valid type for function argument!");
    args_.push_back(p);
  }
}

bool FunctionType::check_return_type(Type *type) {
  return type->is_Integer_type() || type->is_Void_type();
}

bool FunctionType::check_arguement_type(Type *type) {
  return type->is_Integer_type() || type->is_Pointer_type();
}

FunctionType *FunctionType::get(Type *ret, std::vector<Type *> args) {
  return new FunctionType(ret, args);
}

unsigned FunctionType::get_args_num() const { return args_.size(); }

Type *FunctionType::get_args_type(unsigned i) const { return args_[i]; }

Type *FunctionType::get_return_type() const { return ret_; }

ArrayType::ArrayType(Type *contained, unsigned num_elements)
    : Type(Type::Array_type), num_elements_(num_elements) {
  //exit_ifnot(_InvalidElemType_Constructor_ArrayType,isValidElementType(contained) &&"Not a valid type for array element!");
  contained_ = contained;
}

bool ArrayType::check_element_type(Type *type) {
  return type->is_Integer_type() || type->is_Array_type();
}

ArrayType *ArrayType::get(Type *contained, unsigned num_elements) {
  return new ArrayType(contained, num_elements);
}

std::vector<unsigned> ArrayType::get_Dims() const {
  std::vector<unsigned> rets;
  auto elem_ty = contained_;
  rets.push_back(num_elements_);
  while (elem_ty->is_Array_type()) {
    auto arr = static_cast<ArrayType *>(elem_ty);
    rets.push_back(arr->get_elements_num());
    elem_ty = arr->get_element_type();
  }
  return rets;
}

PointerType::PointerType(Type *contained)
    : Type(Type::Pointer_type), contained_(contained) {}

PointerType *PointerType::get(Type *contained) {
  return new PointerType(contained);
}

