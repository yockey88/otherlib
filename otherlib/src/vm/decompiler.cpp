/**
 * \file vm/decompiler.cpp
 **/
#include "vm/decompiler.hpp"

#include <sstream>

#include "core/logger.hpp"

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/control_table.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"
#include "vm/vm.hpp"

namespace other {
  namespace detail {

    uint8_t get_register_byte(const instruction& instr) {
      return instr.bytes[instruction::X_REGISTER_BYTE_IDX];
    }

  }  // namespace detail

  std::string decompiler::opcode_to_string(uint32_t opcode) {
    instruction instr(opcode);
    return "Opcode(Category: " + std::to_string(instr.category_nibble()) +
      ", Type: " + std::to_string(instr.type_nibble()) + ")";
  }

  std::string decompiler::opcode_to_detailed_string(uint32_t opcode) {
    instruction instr(opcode);
    std::string result = "Opcode: ";

    result += std::format("{:#010x}", instr.opcode);
    result += " { ";
    result += "Category: " + std::to_string(instr.category_nibble()) + ", ";
    result += "Type: " + std::to_string(instr.type_nibble()) + ", ";
    result += "Bytes: [ ";
    for (size_t i = 0; i < other_command_device::kOpCodeSize; ++i) {
      result += "0x" + std::to_string(instr.bytes[i]);
      if (i < other_command_device::kOpCodeSize - 1) {
        result += ", ";
      }
    }
    result += " ], ";
    result += "Upper: 0x" + std::to_string(instr.upper) + ", ";
    result += "Lower: 0x" + std::to_string(instr.lower);
    result += " }";

    return result;
  }

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

  void decompiler::dump_instructions(const std::span<const uint8_t> instructions) {
    if (instructions.size() % other_command_device::kOpCodeSize != 0) {
      CORE_LOG_ERROR("Instruction size is not a multiple of opcode size!");
      return;
    }

    other_command_device device = {};
    vm::initialize_device(&device);
    vm::activate_builtin_control_table(&device, OTHER_CONTROL_TABLE_DECOMPILER_V000);

    OTHER_ASSERT(instructions.size() >= sizeof(ocmd_file_header), "Instructions size is smaller than OCMD file header size!");
    const ocmd_file_header& file_header = *(reinterpret_cast<const ocmd_file_header*>(instructions.data()));
    const program_header& prog_header = file_header.prog_header;

    natural_t num_instructions = prog_header.num_instructions;
    CORE_LOG_DEBUG("OCMD File Version: {}.{}", static_cast<int>(file_header.file_version_major), static_cast<int>(file_header.file_version_minor));
    CORE_LOG_DEBUG("Number of Instructions: {}", num_instructions);
    CORE_LOG_DEBUG("Entry Point Address: {:#06x}", prog_header.entry_point_address);

    uint32_t offset = prog_header.code_section_offset + sizeof(ocmd_file_header);
    for (natural_t i = 0; i < num_instructions; ++i) {
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