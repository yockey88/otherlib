/**
 * \file vm/vm_type.cpp
 **/
#include "vm/vm_type.hpp"

namespace other {

  value_type vm_type_to_value(vm_type t) {
    switch (t) {
      case VM_TYPE_VOID: return value_type::EMPTY_TYPE;
      case VM_TYPE_BOOL: return value_type::OEBOOL;
      case VM_TYPE_CHAR: return value_type::CHAR;
      case VM_TYPE_U8: return value_type::UINT8;
      case VM_TYPE_U16: return value_type::UINT16;
      case VM_TYPE_U32: return value_type::UINT32;
      case VM_TYPE_U64: return value_type::UINT64;
      case VM_TYPE_I8: return value_type::INT8;
      case VM_TYPE_I16: return value_type::INT16;
      case VM_TYPE_I32: return value_type::INT32;
      case VM_TYPE_I64: return value_type::INT64;
      case VM_TYPE_F32: return value_type::FLOAT;
      case VM_TYPE_F64: return value_type::DOUBLE;
      case VM_TYPE_PTR:
      case VM_TYPE_HANDLE:
        return value_type::OPAQUE_HANDLE;
    }
    OTHER_ASSERT(false, "unreachable");
    return value_type::EMPTY_TYPE;
  }

  vm_type value_to_vm_type(value_type v) {
    switch (v) {
      case value_type::OEBOOL: return VM_TYPE_BOOL;
      case value_type::CHAR: return VM_TYPE_CHAR;
      case value_type::INT8: return VM_TYPE_I8;
      case value_type::INT16: return VM_TYPE_I16;
      case value_type::INT32: return VM_TYPE_I32;
      case value_type::INT64: return VM_TYPE_I64;
      case value_type::UINT8: return VM_TYPE_U8;
      case value_type::UINT16: return VM_TYPE_U16;
      case value_type::UINT32: return VM_TYPE_U32;
      case value_type::UINT64: return VM_TYPE_U64;
      case value_type::FLOAT: return VM_TYPE_F32;
      case value_type::DOUBLE: return VM_TYPE_F64;
      case value_type::STRING: return VM_TYPE_STRING;
      case value_type::VEC2:
      case value_type::VEC3:
      case value_type::VEC4:
      case value_type::IVEC2:
      case value_type::IVEC3:
      case value_type::IVEC4:
      case value_type::MAT2:
      case value_type::MAT3:
      case value_type::MAT4:
      case value_type::QUATERNION:
      case value_type::SAMPLER2D:
      case value_type::SAMPLER2D_ARRAY:
      case value_type::ASSET:
      case value_type::ENTITY:
      case value_type::USER_TYPE:
      case value_type::OPAQUE_HANDLE:
      case value_type::BYTE_BUFFER:
        return VM_TYPE_PTR;
      case value_type::EMPTY_TYPE:
        return VM_TYPE_VOID;
    }
    OTHER_ASSERT(false, "unreachable");
    return VM_TYPE_VOID;
  }

}  // namespace other