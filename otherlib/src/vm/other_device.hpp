/**
 * \file vm/other_device.hpp
 **/
#ifndef OTHERLIB_VM_OTHER_DEVICE_HPP
#define OTHERLIB_VM_OTHER_DEVICE_HPP

#include "math/random.hpp"

#include "vm/control_table.hpp"
#include "vm/opcode.hpp"
#include "vm/register.hpp"
#include "vm/vm_memory.hpp"

namespace other {

  class scene;
  class driver;

  struct other_command_device {
    constexpr static size_t kOpCodeSize = sizeof(uint32_t);

    constexpr static size_t kMemorySize = 0xFFFF;
    constexpr static size_t kStackSize = 128;

    enum : uint8_t {
      kFlagZero = 1 << 0,
      kFlagCarry = 1 << 1,
      kFlagNegative = 1 << 2,
      kFlagOverflow = 1 << 3,
    };

    enum device_state : uint64_t {
      kStateStopped = 0,
      kStateRunning = 1 << 0,

      kStateError = std::numeric_limits<uint64_t>::max(),
    };

    vm_register registers[vm_register::kNumRegisters + 1];

    using memory_t = memory_storage_t<kMemorySize>;
    memory_t* memory = {};

    /// an array index by instruction category nibble loaded
    ///   that must be loaded before the device can be used
    other_command_executor* control_table = nullptr;

    /// for device-only use
    constexpr static uint16_t kMaxDeviceAddress = kMemorySize;
    constexpr static uint16_t kProgramStartAddress = 0x0010;

    uint16_t program_load_cursor = kProgramStartAddress;
    uint16_t pc = 0;
    uint16_t index = 0;

    uint16_t stack[kStackSize] = {};
    uint8_t sp = 0;

    driver* host_driver = nullptr;
    scene* scene_context = nullptr;

    bool stopped = true;

    instruction current_instruction = 0x0;

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    random_generator<uint64_t> rng = {
      std::numeric_limits<uint8_t>::min(),
      std::numeric_limits<uint8_t>::max(),
    };

    enum program_state : uint64_t {
      kProgramStateStopped = 0,
      kProgramStateIdle = 1 << 0,
      kProgramStateRunning = 1 << 1,

      kProgramStateError = std::numeric_limits<uint64_t>::max(),
    };
    struct program_metadata {
      uint16_t load_address = 0;  // beginning of code section
      uint16_t code_size = 0;

      uint16_t data_offset = 0;  // local program offset of data section
      uint16_t data_size = 0;

      uint16_t entry_point_offset = 0;  // local program offset of entry point (if 0, then load_address)
      uint16_t num_instructions = 0;

      program_state state = kProgramStateStopped;

      uint16_t full_program_size() const;
      uint16_t get_global_data_address() const;
      uint16_t get_global_entry_point_address() const;
      uint16_t get_instruction_address_by_index(uint16_t instruction_index) const;
      uint16_t get_data_address_by_offset(uint16_t data_offset) const;
    };
    program_metadata current_program_metadata;

    inline natural_t read_register_as_u64(uint8_t reg_index) const {
      OTHER_ASSERT(reg_index < vm_register_idx::VM_RFLAG, "Invalid register index: {}", reg_index);
      return registers[reg_index].memory.to_u64();
    }
    inline void write_register_from_u64(uint8_t reg_index, uint64_t value) {
      OTHER_ASSERT(reg_index < vm_register_idx::VM_RFLAG, "Invalid register index: {}", reg_index);
      registers[reg_index].memory = register_t<vm_register::kRegisterBitSize>{ value };
    }

    uint16_t globalize_address(uint16_t local_address) const;
    uint16_t globalize_address(const program_metadata* md, uint16_t local_address) const;
    const void* access_current_program_memory(uint16_t program_offset);
    const void* access_current_program_data_section(uint16_t program_offset);
    natural_t current_program_data_as_u64(uint16_t data_offset) const;

    void write_current_program_memory(uint16_t address, const uint8_t* bytes, size_t length);
    void write_current_program_data(uint16_t address, const uint8_t* bytes, size_t length);
    void write_current_program_data_from_u64(uint16_t data_offset, uint64_t value);

    uint8_t get_random_byte();

    void write_u64_at(const size_t address, const uint64_t value);
    uint64_t read_u64_at(const size_t address) const;
  };

}  // namespace other

#endif  // OTHERLIB_VM_OTHER_DEVICE_HPP