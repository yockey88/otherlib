/**
 * \file vm/decompiler.hpp
 **/
#ifndef OTHERLIB_VM_DECOMPILER_HPP
#define OTHERLIB_VM_DECOMPILER_HPP

#include <cstdint>
#include <span>

namespace other {

  struct program;
  struct other_command_device;

  struct decompiler {
    static void hexdump_memory(other_command_device* device);

    static void dump_instructions(program* prog);
    static void dump_instructions(const std::span<const uint8_t> instructions);
  };

}  // namespace other

#endif  // OTHERLIB_VM_DECOMPILER_HPP