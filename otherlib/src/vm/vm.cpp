/**
 * \file vm/vm.cpp
 **/
#include "vm/vm.hpp"

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"

#include "vm/control_table.hpp"
#include "vm/opcode.hpp"

namespace other {

  void vm::initialize_device(other_command_device* device) {
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

  void vm::shutdown_device(other_command_device* device) {
    if (device == nullptr) {
      return;
    }

    std::ranges::fill(std::span(device->memory->data, other_command_device::kMemorySize), 0);

    arena_allocator<other_command_device::memory_t>{}.free(device->memory);
    device->memory = nullptr;
  }

  void vm::update_device_timers(other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    if (device->delay_timer > 0) {
      --device->delay_timer;
    }
    if (device->sound_timer > 0) {
      --device->sound_timer;
    }
  }

  void vm::step(other_command_device* device) {
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

  void vm::activate_builtin_control_table(other_command_device* device, control_tables table) {
    OTHER_ASSERT(device != nullptr, "Null VM device!");
    load_builtin_control_table(device, table);
  }

  void vm::load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size) {
    OTHER_ASSERT(device, "Device is null!");
    OTHER_ASSERT(device->memory, "Device memory is null");
    OTHER_ASSERT(address < other_command_device::kMemorySize, "Address out of bounds!");
    if (size == 0) {
      return;
    }

    OTHER_ASSERT(data, "Data must not be nulL!");
    for (size_t i = 0; i < size; ++i) {
      device->memory->write_byte(address + i, data[i]);
    }
  }

  void vm::load_program_from_bytes(other_command_device* device, const std::span<const uint8_t> bytes) {
    OTHER_ASSERT(device, "Device is null!");
    OTHER_ASSERT(device->memory, "Device memory is null");

    load_bytes_to_address(device, device->program_load_cursor, bytes.data(), bytes.size());

    device->pc = device->program_load_cursor;
    device->program_load_cursor += bytes.size();
    device->stopped = false;
  }

  void vm::write_instruction_at_address(other_command_device* device, uint64_t address, const instruction& instr) {
    vm::load_bytes_to_address(device, address, reinterpret_cast<const uint8_t*>(&instr.opcode), sizeof(instr.opcode));
  }

}  // namespace other