/**
 * \file vm/command_files/linker.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_LINKER_HPP
#define OTHERLIB_VM_COMMAND_FILES_LINKER_HPP

#include "vm/command_files/assembler.hpp"
#include "vm/other_device.hpp"

#include "assembler.hpp"

namespace other {

  class ocmd_linker {
   public:
    ocmd_linker(const ocmd_assembled_code& assembled_code)
        : assembled_codes(assembled_code) {}
    // ocmd_linker(const std::vector<ocmd_assembled_code>& assembled_codes)
    //     : assembled_codes(assembled_codes) {}
    ~ocmd_linker() = default;

    std::vector<uint8_t> link();

   private:
    ocmd_assembled_code assembled_codes;
    // std::vector<ocmd_assembled_code> assembled_codes = {};
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_LINKER_HPP