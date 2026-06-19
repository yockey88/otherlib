/**
 * \file vm/vm_error.cpp
 **/
#include "vm/vm_error.hpp"

#include "core/enum_formatter.hpp"

namespace other {

  std::string get_vm_error_name(vm_error error) {
    return std::format("{}", error);
  }

}  // namespace other