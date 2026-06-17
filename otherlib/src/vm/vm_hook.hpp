/**
 * \file vm/vm_hook.hpp
 **/
#ifndef OTHERLIB_VM_VM_HOOK_HPP
#define OTHERLIB_VM_VM_HOOK_HPP

#include <span>

#include "vm/vm_function.hpp"

namespace other {

  struct vm_hook_descriptor {
    uint8_t id = 0;
    std::string_view name;
    std::span<const function_descriptor> methods;
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_HOOK_HPP