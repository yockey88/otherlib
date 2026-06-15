/**
 * \file vm/other_device.hpp
 **/
#ifndef OTHERLIB_VM_OTHER_DEVICE_HPP
#define OTHERLIB_VM_OTHER_DEVICE_HPP

#include "math/random.hpp"

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/control_table.hpp"
#include "vm/opcode.hpp"
#include "vm/register.hpp"
#include "vm/vm_memory.hpp"

namespace other {

  class scene;
  class driver;
  class command_bus;

  struct other_command_device {
    constexpr static size_t kOpCodeSize = sizeof(uint32_t);

    constexpr static size_t kMemorySize = 0xFFFF;
    constexpr static size_t kStackSize = 128;
    constexpr static size_t kMaxStringLen = 512;

    enum state : uint8_t {
      STOPPED = 1 << 0,
      INITIALIZED = 1 << 1,

      IDLE = 1 << 2,
      RUNNING = 1 << 3,

      DEBUG = 1 << 4,
      REPL = 1 << 5,

      VM_ERROR = 1 << 7,
    };
    state current_state = STOPPED;

    vm_register registers[vm_register::kNumRegisters + 1];

    using memory_t = memory_storage_t<kMemorySize>;
    memory_t* memory = {};

    /// an array index by instruction category nibble loaded
    ///   that must be loaded before the device can be used
    const other_command_table* control_table = nullptr;

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
    command_bus* bus = nullptr;

    bool stopped = true;

    instruction current_instruction = 0x0;

    uint64_t epoch_time = 0;
    uint64_t device_init_time = 0;
    uint64_t time_since_init = 0;

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    random_generator<uint64_t> rng = {
      std::numeric_limits<uint8_t>::min(),
      std::numeric_limits<uint8_t>::max(),
    };

    enum program_state : uint64_t {
      PROGRAM_STOPPED = 1 << 0,

      PROGRAM_IDLE = 1 << 1,
      PROGRAM_RUNNING = 1 << 2,

      PROGRAM_ERROR = std::numeric_limits<uint64_t>::max(),
    };
    struct program_metadata {
      uint16_t load_address = 0;  // beginning of code section
      uint16_t code_size = 0;
      uint16_t data_size = 0;

      ocmd_file_header header;

      program_state state = PROGRAM_STOPPED;

      uint16_t entry_point() const {
        return header.prog_header.entry_point_address;
      }
    };
    program_metadata current_program_metadata;

    inline natural_t read_register_as_u64(uint8_t reg_index) const {
      OTHER_ASSERT(reg_index <= vm_register_idx::VM_RFLAG, "Invalid register index: {}", reg_index);
      return registers[reg_index].memory.to_u64();
    }
    inline void write_register_from_u64(uint8_t reg_index, uint64_t value) {
      OTHER_ASSERT(reg_index <= vm_register_idx::VM_RFLAG, "Invalid register index: {}", reg_index);
      registers[reg_index].memory = register_t<vm_register::kRegisterBitSize>{ value };
    }
    inline uint64_t read_flag_register() const {
      return read_register_as_u64(vm_register::kFlagRegister);
    }
    inline void write_flag_register(uint64_t value) {
      write_register_from_u64(vm_register::kFlagRegister, value);
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