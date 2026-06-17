/**
 * \file vm/decompiler.hpp
 **/
#ifndef OTHERLIB_VM_DECOMPILER_HPP
#define OTHERLIB_VM_DECOMPILER_HPP

#include <cstdint>
#include <span>
#include <string>

namespace other {

  struct other_command_device;

  struct decompiler {
    static std::string register_byte_name(uint8_t reg);
    static std::string get_instruction_name(uint32_t opcode);
    static std::string opcode_to_string(uint32_t opcode);
    static std::string opcode_to_detailed_string(uint32_t opcode);
    static void hexdump_memory(other_command_device* device);

    static void dump_instructions(const std::span<const uint8_t> instructions);
  };

}  // namespace other

#endif  // OTHERLIB_VM_DECOMPILER_HPP