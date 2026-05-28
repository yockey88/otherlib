/**
 * \file vm/vm.cpp
 **/
#include "vm/vm.hpp"

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"
#include "thread/thread_safety.hpp"

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/control_table.hpp"
#include "vm/opcode.hpp"

namespace other {

  void vm::initialize_device(other_command_device* device) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device != nullptr, "Null device!");

    device->memory = arena_allocator<other_command_device::memory_t>{}.allocate();
    OTHER_ASSERT(device->memory != nullptr, "Failed to allocate device memory!");

    auto data = std::span(device->memory->data, other_command_device::kMemorySize);
    std::ranges::fill(data, 0);

    device->program_load_cursor = device->kProgramStartAddress;
    device->pc = device->kProgramStartAddress;
    device->index = 0;

    device->stopped = true;

    device->sp = 0;

    /// can be overridden later if needed
    load_builtin_control_table(device, OTHER_CONTROL_TABLE_V000);
  }

  void vm::load_control_table(other_command_device* device, control_tables table) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device != nullptr, "Null VM device!");
    load_builtin_control_table(device, table);
  }

  void vm::load_program_from_bytes(other_command_device* device, const std::span<const uint8_t> bytes) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device, "Device is null!");
    OTHER_ASSERT(device->memory, "Device memory is null");

    /// \todo we may not want to run it right away?
    ocmd_file_header header = *reinterpret_cast<const ocmd_file_header*>(bytes.data());
    if (header.file_signature[0] != 'O' || header.file_signature[1] != 'C' || header.file_signature[2] != 'M' || header.file_signature[3] != 'D') {
      CORE_LOG_ERROR("Invalid OCMD file signature!");
      return;
    }

    device->current_program_metadata = {
      .load_address = device->program_load_cursor,
      .code_size = header.prog_header.code_size,
      .data_offset = static_cast<uint16_t>(header.prog_header.data_section_offset - sizeof(ocmd_file_header)),
      .data_size = header.prog_header.data_size,
      .entry_point_offset = header.prog_header.entry_point_address,
      .num_instructions = header.prog_header.num_instructions,
      .state = other_command_device::program_state::kProgramStateRunning,
    };

    auto program_bytes = bytes.subspan(sizeof(ocmd_file_header));
    size_t program_size = std::ranges::size(program_bytes);
    const uint8_t* program = program_bytes.data();
    load_bytes_to_address(device, device->program_load_cursor, program, program_size);

    device->pc = device->program_load_cursor;
    device->program_load_cursor += program_size;
    device->stopped = false;
  }

  void vm::step(other_command_device* device) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device != nullptr, "Null VM device!");
    OTHER_ASSERT(device->memory != nullptr, "Null VM device memory!");
    if (device->stopped) {
      return;
    }

    device->current_instruction = { opcode_read_program_counter_and_shift(device) };
    uint8_t instr_nib = device->current_instruction.category_nibble();

    device->control_table[instr_nib](device);

    update_device_timers(device);
  }

  void vm::shutdown_device(other_command_device* device) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr) {
      return;
    }

    std::ranges::fill(std::span(device->memory->data, other_command_device::kMemorySize), 0);

    arena_allocator<other_command_device::memory_t>{}.free(device->memory);
    device->memory = nullptr;
  }

  natural_t vm::get_register_as_u64(other_command_device* device, uint8_t reg_idx) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr) {
      return 0;
    }
    return device->read_register_as_u64(reg_idx);
  }

  void vm::write_register_from_u64(other_command_device* device, uint8_t reg_idx, uint64_t value) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr) {
      return;
    }
    device->write_register_from_u64(reg_idx, value);
  }

  void* vm::register_memory(other_command_device* device, uint8_t reg_idx) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr) {
      return nullptr;
    }
    if (reg_idx >= vm_register_idx::VM_RFLAG) {
      return nullptr;
    }
    return device->registers[reg_idx].memory.memory();
  }

  const void* vm::register_memory(const other_command_device* device, uint8_t reg_idx) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr) {
      return nullptr;
    }
    if (reg_idx >= vm_register_idx::VM_RFLAG) {
      return nullptr;
    }
    return device->registers[reg_idx].memory.memory();
  }

  void vm::write_u64_at(other_command_device* device, uint64_t address, uint64_t value) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr || device->memory == nullptr) {
      return;
    }

    std::memcpy(device->memory->data + address, &value, sizeof(uint64_t));
  }

  uint64_t vm::read_u64_at(const other_command_device* device, uint64_t address) {
    ASSERT_MAIN_THREAD();
    if (device == nullptr || device->memory == nullptr) {
      return 0;
    }

    uint64_t value;
    std::memcpy(&value, device->memory->data + address, sizeof(uint64_t));
    return value;
  }

  void* vm::memory_pointer(other_command_device* device, uint64_t address) {
    if (device == nullptr || device->memory == nullptr) {
      return nullptr;
    }
    if (address >= other_command_device::kMemorySize) {
      return nullptr;
    }
    return device->memory->data + address;
  }

  const void* vm::memory_pointer(const other_command_device* device, uint64_t address) {
    if (device == nullptr || device->memory == nullptr) {
      return nullptr;
    }
    if (address >= other_command_device::kMemorySize) {
      return nullptr;
    }
    return device->memory->data + address;
  }

  void vm::update_device_timers(other_command_device* device) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device != nullptr, "Null device!");
    if (device->delay_timer > 0) {
      --device->delay_timer;
    }
    if (device->sound_timer > 0) {
      --device->sound_timer;
    }
  }

  void vm::load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(device, "Device is null!");
    OTHER_ASSERT(device->memory, "Device memory is null");
    OTHER_ASSERT(address + size < other_command_device::kMemorySize, "Address out of bounds!");
    if (size == 0) {
      return;
    }

    OTHER_ASSERT(data, "Data must not be null!");
    std::memcpy(device->memory->data + address, data, size);
  }

  void vm::write_instruction_at_address(other_command_device* device, uint64_t address, const instruction& instr) {
    ASSERT_MAIN_THREAD();
    vm::load_bytes_to_address(device, address, reinterpret_cast<const uint8_t*>(&instr.opcode), sizeof(instr.opcode));
  }

}  // namespace other