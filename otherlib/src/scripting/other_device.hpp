/**
 * \file scripting/other_device.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_OTHER_DEVICE_HPP
#define OTHERLIB_SCRIPTING_OTHER_DEVICE_HPP

#include <bitset>

#include "core/defines.hpp"
#include "core/value.hpp"
#include "math/random.hpp"

namespace other {

  struct program;

  struct other_command_device;
  struct execution_context;
  using handler = void (*)(other_command_device*, execution_context*);

  template <size_t N>
  using register_t = std::bitset<N>;

  template <size_t N>
  struct memory_storage_t {
    constexpr static size_t size() { return N; }
    uint8_t data[N];

    constexpr memory_storage_t() : data{} {
      std::ranges::fill(std::span(data, N), 0);
    }

    uint8_t& at(const size_t index) { return *checked_get_ptr_at(index); }
    const uint8_t& at(const size_t index) const { return *checked_get_ptr_at(index); }
    uint8_t* checked_get_ptr_at(const size_t index) {
      assert(index < N && "Index out of bounds");
      return &data[index];
    }

    void write_byte(const size_t index, const uint8_t value) {
      assert(index < size() && "Index out of bounds");
      data[index] = value;
    }
    uint8_t read_byte(const size_t index) const {
      assert(index < size() && "Index out of bounds");
      return data[index];
    }

    void* start() { return data; }
    void* end() { return data + N; }
    void* unsafe_at(const size_t index) { return data + index; }

    void* ptr_to(const size_t index) {
      assert(index < N && "Index out of bounds");
      return unsafe_at(index);
    }
  };

  struct instruction {
    constexpr static uint32_t kCategoryMask = 0xF0000000;
    constexpr static uint8_t kCategoryShift = 28;

    constexpr static uint32_t kTypeMask = 0x0F000000;
    constexpr static uint8_t kTypeShift = 24;

    constexpr static uint32_t kUpperHalfwordMask = 0xFFFF0000;
    constexpr static uint32_t kUpperHalfwordShift = sizeof(uint16_t);

    constexpr static uint32_t kLowerHalfwordMask = 0x0000FFFF;
    constexpr static uint32_t kLowerHalfwordShift = 0;

    constexpr static uint32_t kXRegisterMask = 0x00FF0000;
    constexpr static uint8_t kXRegisterShift = 16;

    enum : uint8_t {
      X_REGISTER_BYTE_IDX = 2,
    };

    union {
      uint32_t opcode = 0;
      union {
        struct {
          uint16_t lower;
          uint16_t upper;
        };
        struct {
          uint8_t bytes[sizeof(uint32_t)];
        };
      };
    };

    uint8_t category_nibble() const { return static_cast<uint8_t>((opcode & kCategoryMask) >> kCategoryShift); }
    uint8_t type_nibble() const { return static_cast<uint8_t>((opcode & kTypeMask) >> kTypeShift); }
  };

  enum control_tables {
    CONTROL_TABLE_V000 = 0,

    NUM_CONTROL_TABLES,
    INVALID_CONTROL_TABLE = NUM_CONTROL_TABLES,
  };

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

    constexpr static size_t kFlagRegister = kNumRegisters;
    /// last register is the flag register
    register_t<kRegisterBitSize> registers[kNumRegisters + 1] = { 0b0 };

    using memory_t = memory_storage_t<kMemorySize>;
    memory_t* memory = {};

    handler* control_table = nullptr;

    /// for device-only use
    constexpr static size_t kMaxDeviceAddress = 0x000000000000000F;
    constexpr static size_t kProgramStartAddress = kMaxDeviceAddress + 1;
    uint64_t program_load_cursor = kProgramStartAddress;
    uint64_t pc = 0;
    uint64_t index = 0;

    uint64_t stack[kStackSize] = {};
    uint8_t sp = 0;

    uint64_t context_stack[kStackSize] = {};
    uint8_t csp = 0;

    bool stopped = true;

    instruction current_instruction = { 0x0 };

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    random_generator<uint64_t> rng{ std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max() };
    uint8_t get_random_byte();
    void write_u64_at(const size_t address, const uint64_t value);
    uint64_t read_u64_at(const size_t address);

    void push_state(const uint64_t state);
    uint64_t pop_state();
  };

  struct execution_context {
    constexpr static size_t kNumRegisters = 16;
    constexpr static size_t kMaxFunctions = 512;

    constexpr static size_t kDynamicMemorySize = 64;
    constexpr static size_t kStackMemorySize = kDynamicMemorySize - kNumRegisters;
    constexpr static size_t kHeapMemorySize = 64;

    constexpr static size_t kMemorySize = kDynamicMemorySize + kHeapMemorySize;

    constexpr static size_t kRegisterStartIdx = 0;
    constexpr static size_t kRegisterEndIdx = kRegisterStartIdx + kNumRegisters;

    constexpr static size_t kStackStartIdx = kRegisterEndIdx;
    constexpr static size_t kStackEndIdx = kStackStartIdx + kStackMemorySize;

    constexpr static size_t kHeapStartIdx = kDynamicMemorySize;
    constexpr static size_t kHeapEndIdx = kHeapStartIdx + kHeapMemorySize;

    struct function {
      uint64_t address = 0;
      std::function<void(execution_context*)> exec = nullptr;
    };

    uint16_t sp = kStackStartIdx;

    void push_argument(const value& val) {
      OTHER_ASSERT(sp < kHeapStartIdx, "Stack overflow in execution context!");
      memory[sp] = val;
      ++sp;
    }

    value& pop_argument() {
      OTHER_ASSERT(sp > kStackStartIdx, "Stack underflow in execution context!");
      --sp;
      return memory[sp];
    }

    std::array<value, kMemorySize> memory;
    std::array<function, kMaxFunctions> functions;
  };

  void initialize_device(other_command_device* device);
  void shutdown_device(other_command_device* device);
  void hexdump_memory(other_command_device* device);
  void dump_instructions(const std::vector<uint8_t>& instructions, uint64_t start_address = 0);

  void set_builtin_control_table(other_command_device* device, control_tables table);
  void load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size);
  void load_program(other_command_device* device, program* progr, bool dump_program = false);

  instruction step_device_one_instruction(other_command_device* device);
  void end_device_cycle(other_command_device* device);

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category);
  uint32_t opcode_set_type(uint32_t opcode, uint8_t type);
  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x);

  uint32_t opcode_set_upper(uint32_t opcode, uint16_t upper);
  uint32_t opcode_set_lower(uint32_t opcode, uint16_t lower);

  /// 0 table (device control)
  /// 0x00000000
  uint32_t opcode_stop_device();
  /// 0x01000000
  uint32_t opcode_dump_registers();
  /// 0x02xx0000
  uint32_t opcode_dump_register_x(uint8_t x);

  /// 1 table (load/store)
  /// 10xxnnnn
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n);
  /// 11xxnnnn
  uint32_t opcode_load_x_from(uint8_t x, uint16_t n);
  /// 12xxnnnn
  uint32_t opcode_load_x_direct(uint8_t x, uint16_t n);
  /// 13xxnnnn
  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x);
  /// 14xxnnnn
  uint32_t opcode_write_x_to_environment_memory(uint8_t x, uint16_t n);
  /// 15xxnnnn
  uint32_t opcode_load_x_from_environment_memory(uint8_t x, uint16_t n);

  /// 2 table (program flow)
  /// 2000nnnn
  uint32_t opcode_goto(uint64_t n);
  /// 21xxnnnn
  uint32_t opcode_goto_if_zero(uint64_t n);
  /// 22xxnnnn
  uint32_t opcode_goto_if_x_zero(uint32_t x, uint64_t n);
  /// 23xxnnnn
  uint32_t opcode_call_at(uint64_t n);
  /// 24000000
  uint32_t opcode_return();
  /// 25xx0000
  uint32_t opcode_return_value_in_x(uint32_t x);

  /// 3 table (environment control)
  /// 3000nnnn
  uint32_t opcode_execute_function_at(uint16_t n);

  struct program {
    struct function {
      std::string name;
      uint16_t address = 0;
      uint16_t compiled_address = 0;
      std::vector<uint8_t> code = {};
    };
    struct call_instruction {
      uint16_t code_offset;
      uint16_t from_address;
      std::string name;
    };

    function* current_function = nullptr;

    std::vector<function> functions = {};
    std::vector<call_instruction> calls = {};

    // device assumes two opcodes at the start of memory for initial jump to main and stop device after main returns
    constexpr static uint64_t kDeviceProgramStartCodeOffset = 2 * sizeof(uint32_t);
    uint16_t current_offset = 0;

    uint32_t num_instructions = 0;

    void start_function(const std::string_view name);
    void end_function();

    void dump_x(uint8_t x);

    void write_x_to_memory(uint8_t x, uint16_t n);
    void load_x_from(uint8_t x, uint16_t n);
    void load_x_direct(uint8_t x, uint16_t n);
    void indirect_write_to(uint16_t n, uint8_t x);
    void write_x_to_environment(uint8_t x, uint16_t n);
    void load_x_from_environment(uint8_t x, uint16_t n);

    void call(const std::string_view name);

    void execute_function_at(uint16_t n);

    void dump_program() const;
    std::vector<uint8_t> compile_program(const uint64_t start_address);

   private:
    void add_opcode(uint32_t opcode);
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_OTHER_DEVICE_HPP