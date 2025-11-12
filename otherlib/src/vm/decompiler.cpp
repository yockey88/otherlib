/**
 * \file vm/decompiler.cpp
 **/
#include "vm/decompiler.hpp"

#include <sstream>

#include "core/logger.hpp"

#include "vm/opcode.hpp"
#include "vm/other_device.hpp"
#include "vm/program.hpp"
#include "vm/vm.hpp"

#include "control_table.hpp"

namespace other {
  namespace detail {

    uint8_t get_register_byte(const instruction& instr) {
      return instr.bytes[instruction::X_REGISTER_BYTE_IDX];
    }

  }  // namespace detail

  void decompiler::hexdump_memory(other_command_device* device) {
    assert(device != nullptr && "Null device!");
    std::stringstream ss;
    for (size_t i = 0; i < other_command_device::kMemorySize; ++i) {
      if (i % 16 == 0) {
        if (i != 0) {
          ss << "\n";
        }
        ss << std::format("[{:#06x}] ", i);
      }
      ss << std::format("{:#04x} ", device->memory->at(i));
    }
    CORE_LOG_DEBUG("{}", ss.str());
  }

  void decompiler::dump_instructions(program* prog) {
    OTHER_ASSERT(prog != nullptr, "Null program!");

    std::vector<uint8_t> instructions = prog->compile_program(0);
    dump_instructions(instructions);
  }

  void decompiler::dump_instructions(const std::span<const uint8_t> instructions) {
    if (instructions.size() % other_command_device::kOpCodeSize != 0) {
      CORE_LOG_ERROR("Instruction size is not a multiple of opcode size!");
      return;
    }

    other_command_device device = {};
    vm::initialize_device(&device);

    vm::activate_builtin_control_table(&device, OTHER_CONTROL_TABLE_DECOMPILER_V000);

    for (uint32_t offset = 0; offset < instructions.size();) {
      const uint32_t* code_ptr = reinterpret_cast<const uint32_t*>(instructions.data() + offset);

      device.current_instruction = *code_ptr;
      device.pc = offset;
      offset += other_command_device::kOpCodeSize;

      uint8_t instr_nib = device.current_instruction.category_nibble();
      device.control_table[instr_nib](&device);
    }

    vm::shutdown_device(&device);
  }

}  // namespace other