/**
 * \file vm/decompiler.cpp
 **/
#include "vm/decompiler.hpp"

#include <sstream>
#include <string>

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

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

  std::string decompiler::register_byte_name(uint8_t reg) {
    switch (reg) {
      case instruction::X_REGISTER_BYTE_IDX: return "X";
      case instruction::Y_REGISTER_BYTE_IDX: return "Y";
      case instruction::Z_REGISTER_BYTE_IDX: return "Z";
      default: return "Unknown";
    }
  }

  std::string decompiler::get_instruction_name(uint32_t opcode) {
    instruction instr(opcode);

    std::string res;
    switch (instr.category_nibble()) {
      case 0: res += "Control"; break;
      case 1: res += "Memory/Logic"; break;
      case 2: res += "Program Flow"; break;
      case 3: res += "Arithmetic"; break;
      case 4: res += "Logic and Bitwise"; break;
      default: return res;
    }

    res += ": ";
    switch (instr.category_nibble()) {
      case 0: {
        switch (instr.type_nibble()) {
          case 0x00: res += "Stop Device"; break;
          case 0x01: res += "Dump Registers"; break;
          case 0x02: res += "Dump Register X"; break;
          case 0x03: res += "Dump reg(X) bytes at N"; break;
          case 0x04: res += "View VM Device State"; break;
          case 0x05: res += "No Op"; break;
          case 0x06: res += "Clear X through Y"; break;
          default:
            OTHER_ASSERT(false, "Invalid type nibble : category = {}, type = {}", instr.category_nibble(), instr.type_nibble());
            break;
        }
      } break;
      case 1: {
        switch (instr.type_nibble()) {
          case 0x00: res += "Move X to Y"; break;
          case 0x01: res += "Write X to memory at N"; break;
          case 0x02: res += "Write X to address in Y"; break;
          case 0x03: res += "Set X to address N"; break;
          case 0x04: res += "Set X to dword at N"; break;
          case 0x05: res += "Set X to K"; break;
          default:
            OTHER_ASSERT(false, "Invalid type nibble : category = {}, type = {}", instr.category_nibble(), instr.type_nibble());
            break;
        }
      } break;
      case 2: {
        switch (instr.type_nibble()) {
          case 0x00: res += "Goto address N"; break;
          case 0x01: res += "Jump to N if FLAG == 0"; break;
          case 0x02: res += "Jump to N if FLAG != 0"; break;
          case 0x03: res += "Call address N"; break;
          case 0x04: res += "Return"; break;
          case 0x05: res += "Return storing result in X"; break;
          case 0x06: res += "System call with id K"; break;
          default:
            OTHER_ASSERT(false, "Invalid type nibble : category = {}, type = {}", instr.category_nibble(), instr.type_nibble());
            break;
        }
      } break;
      case 3: {
        switch (instr.type_nibble()) {
          case 0x00: res += "Set X = X + Y"; break;
          case 0x01: res += "Set X = X - Y"; break;
          case 0x02: res += "Set X = X * Y"; break;
          case 0x03: res += "Set X = X / Y"; break;
          case 0x04: res += "Set X = X % Y"; break;
          case 0x05: res += "Set X = X + K"; break;
          case 0x06: res += "Set X = X - K"; break;
          case 0x07: res += "Set X = X * K"; break;
          case 0x08: res += "Set X = X / K"; break;
          case 0x09: res += "Set X = X % K"; break;
          default:
            OTHER_ASSERT(false, "Invalid type nibble : category = {}, type = {}", instr.category_nibble(), instr.type_nibble());
            break;
        }
      } break;
      case 4: {
        switch (instr.type_nibble()) {
          case 0x00: res += "Set Z = X == Y"; break;
          case 0x01: res += "Set Z = X > Y"; break;
          case 0x02: res += "Set Z = X < Y"; break;
          case 0x03: res += "Set Z = X & Y"; break;
          case 0x04: res += "Set Z = X | Y"; break;
          case 0x05: res += "Set Z = X ^ Y"; break;
          case 0x06: res += "Set X = X << Y"; break;
          case 0x07: res += "Set X = X >> Y"; break;
          default:
            OTHER_ASSERT(false, "Invalid type nibble : category = {}, type = {}", instr.category_nibble(), instr.type_nibble());
            break;
        }
      } break;
      default:
        OTHER_ASSERT(false, "Invalid category nibble : category = {}", instr.category_nibble());
    }

    return res;
  }

  std::string decompiler::opcode_to_string(uint32_t opcode) {
    instruction instr(opcode);
    return "Opcode(Category: " + std::to_string(instr.category_nibble()) + ", Type: " + std::to_string(instr.type_nibble()) + ")";
  }

  std::string decompiler::opcode_to_detailed_string(uint32_t opcode) {
    instruction instr(opcode);
    std::string result = std::format("Opcode [{}]: ", get_instruction_name(opcode));
    result += std::format("{:#010x}", instr.opcode);
    result += " {\n";
    result += "  Category: " + std::to_string(instr.category_nibble()) + ",\n";
    result += "  Type: " + std::to_string(instr.type_nibble()) + ",\n";
    result += std::format("  N/K: {:#06x},\n", instr.lower);
    result += "  Registers: [\n    ";
    for (size_t i = 0; i < instruction::INVALID_BYTE_IDX; ++i) {
      result += std::format("{} = {:#02x} ", register_byte_name(i), instr.bytes[i]);
      if (i < instruction::Z_REGISTER_BYTE_IDX) {
        result += ", ";
      }
    }
    result += "\n  ],\n";
    result += "}";

    return result;
  }

  void decompiler::hexdump_memory(other_command_device* device) {
    assert(device != nullptr && "Null device!");
    PROFILE_SECTION("decompiler::hexdump_memory");
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
    PROFILE_SECTION("decompiler::dump_instructions");
    if (instructions.size() % other_command_device::kOpCodeSize != 0) {
      CORE_LOG_ERROR("Instruction size is not a multiple of opcode size!");
      return;
    }

    other_command_device device = {};
    vm::initialize_device(&device);
    vm::load_control_table(&device, OTHER_CONTROL_TABLE_DECOMPILER_V000);

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
      (*device.control_table)[instr_nib](&device);
    }

    vm::shutdown_device(&device);
  }

}  // namespace other