/**
 * \file vm/command_files/ocmd_toolchain.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OCMD_TOOLCHAIN_HPP
#define OTHERLIB_VM_COMMAND_FILES_OCMD_TOOLCHAIN_HPP

#include "vm/command_files/ocmd_headers.hpp"
#include "data-structures/std_container.hpp"
#include "vm/vm_version.hpp"

namespace other {

  struct other_command_device;

  class ocmd_toolchain {
   public:
    ostd::vector<uint8_t> assemble_oasm_source(other_command_device* device, const filepath& path);
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OCMD_TOOLCHAIN_HPP