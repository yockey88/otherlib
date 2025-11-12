/**
 * \file vm/other_device.hpp
 **/
#ifndef OTHERLIB_VM_OTHER_DEVICE_HPP
#define OTHERLIB_VM_OTHER_DEVICE_HPP

#include "math/random.hpp"

#include "vm/control_table.hpp"
#include "vm/opcode.hpp"
#include "vm/vm_memory.hpp"

namespace other {

  struct program;

  struct other_command_device {
    constexpr static size_t kOpCodeSize = sizeof(uint32_t);
    constexpr static size_t kRegisterBitSize = 64;  /// 64-bit registers
    constexpr static size_t kNumRegisters = 16;
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

    constexpr static uint8_t kReturnRegister = 15;
    constexpr static size_t kFlagRegister = kNumRegisters;
    /// last register is the flag register
    register_t<kRegisterBitSize> registers[kNumRegisters + 1];

    using memory_t = memory_storage_t<kMemorySize>;
    memory_t* memory = {};

    /// an array index by instruction category nibble loaded
    ///   that must be loaded before the device can be used
    other_command_executor* control_table = nullptr;

    /// for device-only use
    constexpr static size_t kMaxDeviceAddress = 0x000000000000000F;
    constexpr static size_t kProgramStartAddress = kMaxDeviceAddress + 1;

    uint64_t program_load_cursor = kProgramStartAddress;
    uint64_t pc = 0;
    uint64_t index = 0;

    uint64_t stack[kStackSize] = {};
    uint8_t sp = 0;

    uint64_t environment_stack[kStackSize] = {};
    uint8_t env_sp = 0;

    bool stopped = true;

    instruction current_instruction = 0x0;

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    random_generator<uint64_t> rng{ std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max() };

    uint8_t get_random_byte();

    void write_u64_at(const size_t address, const uint64_t value);
    uint64_t read_u64_at(const size_t address);

    // void push_state(const uint64_t state);
    // uint64_t pop_state();
  };

}  // namespace other

#endif  // OTHERLIB_VM_OTHER_DEVICE_HPP