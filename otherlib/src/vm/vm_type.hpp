/**
 * \file vm/vm_type.hpp
 **/
#ifndef OTHERLIB_VM_VM_TYPE_HPP
#define OTHERLIB_VM_VM_TYPE_HPP

namespace other {

  enum vm_type : uint8_t {
    VM_TYPE_VOID,
    VM_TYPE_BOOL,
    VM_TYPE_CHAR,
    VM_TYPE_U8,
    VM_TYPE_U16,
    VM_TYPE_U32,
    VM_TYPE_U64,
    VM_TYPE_I8,
    VM_TYPE_I16,
    VM_TYPE_I32,
    VM_TYPE_I64,
    VM_TYPE_F32,
    VM_TYPE_F64,
    VM_TYPE_STRING,
    VM_TYPE_PTR,     // 16-bit device address
    VM_TYPE_HANDLE,  // opaque u64 returned by another syscall
  };

  struct type_descriptor {
    vm_type kind;
    std::string_view name;
  };

  value_type vm_type_to_value(vm_type t);
  vm_type value_to_vm_type(value_type v);

}  // namespace other

#endif  // OTHERLIB_VM_VM_TYPE_HPP