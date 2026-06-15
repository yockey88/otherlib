/**
 * \file vm/vm_function.hpp
 **/
#ifndef OTHERLIB_VM_VM_FUNCTION_HPP
#define OTHERLIB_VM_VM_FUNCTION_HPP

#include <span>

#include "vm/vm_type.hpp"

namespace other {

  enum side_effect_flag : uint8_t {
    VM_SE_PURE = 0,
    VM_SE_READS_HOST = 1 << 0,
    VM_SE_WRITES_HOST = 1 << 1,
    VM_SE_MAY_TRAP = 1 << 2,
    VM_SE_MAY_BLOCK = 1 << 3,          // more than ~1us
    VM_SE_ANYTHING_POSSIBLE = 1 << 4,  // impossible to tell what could happen
  };

  struct function_descriptor {
    uint8_t id;
    std::string_view name;
    std::span<const type_descriptor> params;  // r0, r1, r2, ...
    type_descriptor returns;                  // type written to RF
    uint8_t side_effects = VM_SE_PURE;
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_FUNCTION_HPP