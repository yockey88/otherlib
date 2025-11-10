/**
 * \file actions/other_device.cpp
 **/
#include "scripting/other_device.hpp"

#include "core/logger.hpp"

namespace other {
  namespace detail {

    void zero_memory(other_command_device* device) {
      std::ranges::fill(std::span(device->memory->data, other_command_device::kMemorySize), 0);
    }

  }  // namespace detail

  uint8_t other_command_device::get_random_byte() {
    return static_cast<uint8_t>(rng.next());
  }

  void other_command_device::write_u64_at(const size_t address, const uint64_t value) {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    *reinterpret_cast<uint64_t*>(memory->unsafe_at(address)) = value;
  }

  uint64_t other_command_device::read_u64_at(const size_t address) {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    return *reinterpret_cast<uint64_t*>(memory->unsafe_at(address));
  }

  namespace detail {

    uint8_t* byte_ptr_at(other_command_device* device) {
      return &device->memory->at(device->pc);
    }

    uint32_t& read_bytes_as_u128(uint8_t* bytes) {
      return *reinterpret_cast<uint32_t*>(std::span(bytes, sizeof(uint32_t)).data());
    }

    uint32_t read_program_counter_and_shift(other_command_device* device) {
      uint32_t value = read_bytes_as_u128(byte_ptr_at(device));
      device->pc += other_command_device::kOpCodeSize;
      return value;
    }

    instruction read_current_instruction(other_command_device* device) {
      return instruction{ read_program_counter_and_shift(device) };
    }

    void update_timers(other_command_device* device) {
      if (device->delay_timer > 0) {
        --device->delay_timer;
      }
      if (device->sound_timer > 0) {
        --device->sound_timer;
      }
    }

    uint8_t get_register_byte(const instruction& instr) {
      return instr.bytes[instruction::X_REGISTER_BYTE_IDX];
    }

    handler* get_builtin_control_table_handler(control_tables table);

  }  // namespace detail

  void initialize_device(other_command_device* device) {
    assert(device != nullptr && "Null device!");
    device->memory = (other_command_device::memory_t*)malloc(sizeof(other_command_device::memory_t));
    detail::zero_memory(device);

    device->program_load_cursor = device->kProgramStartAddress;
    device->pc = device->kProgramStartAddress;
    device->index = 0;

    device->stopped = false;

    device->sp = 0;
  }

  void shutdown_device(other_command_device* device) {
    detail::zero_memory(device);

    free(device->memory);
    device->memory = nullptr;
  }

  void hexdump_memory(other_command_device* device) {
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

  void dump_instructions(const std::vector<uint8_t>& instructions, uint64_t start_address) {
    for (size_t offset = 0; offset < instructions.size(); offset += sizeof(uint32_t)) {
      if (offset + sizeof(uint32_t) > instructions.size()) {
        break;
      }
      instruction instr{ *reinterpret_cast<const uint32_t*>(&instructions[offset]) };
      CORE_LOG_DEBUG("[{:#06x}] : {:#010x} [{:#x}:{:#x}]", start_address + offset, instr.opcode, instr.category_nibble(), instr.type_nibble());

      switch (instr.category_nibble()) {
        case 0x0: {
          switch (instr.type_nibble()) {
            case 0x0:
              CORE_LOG_DEBUG("  [STOP-DEVICE]");
              break;

            case 0x1:
              CORE_LOG_DEBUG("  [DUMP-REGISTERS]");
              break;

            case 0x2:
              CORE_LOG_DEBUG("  [DUMP-REGISTER-X] x={}", detail::get_register_byte(instr));
              break;

            default:
              CORE_LOG_DEBUG("  [UNKNOWN]");
          }
          break;
        }

        case 0x1: {
          switch (instr.type_nibble()) {
            case 0x0:
              CORE_LOG_DEBUG("  [WRITE-X-TO-MEMORY] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x1:
              CORE_LOG_DEBUG("  [LOAD-X-FROM-MEMORY] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x2:
              CORE_LOG_DEBUG("  [LOAD-X-DIRECT] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x3:
              CORE_LOG_DEBUG("  [INDIRECT-WRITE-X-TO-MEMORY] n={:#06x} x={}", instr.lower, detail::get_register_byte(instr));
              break;

            case 0x4:
              CORE_LOG_DEBUG("  [WRITE-X-TO-ENV-MEMORY] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x5:
              CORE_LOG_DEBUG("  [LOAD-X-FROM-ENV-MEMORY] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x6:
              CORE_LOG_DEBUG("  [INDIRECT-WRITE-X-TO-ENV-MEMORY] n={:#06x} x={}", instr.lower, detail::get_register_byte(instr));
              break;

            case 0x7:
              CORE_LOG_DEBUG("  [INDIRECT-LOAD-X-FROM-ENV-MEMORY] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            default:
              CORE_LOG_DEBUG("  [UNKNOWN]");
          }
          break;
        }

        case 0x2: {
          switch (instr.type_nibble()) {
            case 0x0:
              CORE_LOG_DEBUG("  [GOTO] n={:#06x}", instr.lower);
              break;

            case 0x1:
              CORE_LOG_DEBUG("  [GOTO-IF-ZERO] n={:#06x}", instr.lower);
              break;

            case 0x2:
              CORE_LOG_DEBUG("  [GOTO-IF-X-ZERO] x={} n={:#06x}", detail::get_register_byte(instr), instr.lower);
              break;

            case 0x3:
              CORE_LOG_DEBUG("  [CALL-AT] n={:#06x}", instr.lower);
              break;

            case 0x4:
              CORE_LOG_DEBUG("  [RETURN]");
              break;

            case 0x5:
              CORE_LOG_DEBUG("  [RETURN-VALUE-IN-X] x={}", detail::get_register_byte(instr));
              break;

            default:
              CORE_LOG_DEBUG("  [UNKNOWN]");
          }
          break;
        }

        case 0x3: {
          switch (instr.type_nibble()) {
            case 0x0:
              CORE_LOG_DEBUG("  [EXECUTE-FUNCTION-AT] n={:#06x}", instr.lower);
              break;

            default:
              CORE_LOG_DEBUG("  [UNKNOWN]");
          }
        } break;

        case 0xF: {
          switch (instr.type_nibble()) {
            case 0xF:
              CORE_LOG_DEBUG("  [UNRESOLVED-CALL]");
              break;

            default:
              CORE_LOG_DEBUG("  [UNKNOWN]");
          }
        } break;

        default:
          CORE_LOG_DEBUG("  [UNKNOWN]");
      }
    }
  }

  void set_builtin_control_table(other_command_device* device, control_tables table) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    device->control_table = detail::get_builtin_control_table_handler(table);
  }

  void load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size) {
    assert(device && "Device is null!");
    assert(device->memory && "Device memory is null");
    assert(address < other_command_device::kMemorySize && "Address out of bounds!");
    if (size == 0) {
      return;
    }

    assert(data && "Data must not be nulL!");
    for (size_t i = 0; i < size; ++i) {
      device->memory->write_byte(address + i, data[i]);
    }
  }

  void load_program(other_command_device* device, program* progr, bool dump_program) {
    assert(device && "Device is null!");
    assert(device->memory && "Device memory is null");
    assert(progr && "Program is null!");

    auto code = progr->compile_program(device->program_load_cursor);
    if (dump_program) {
      dump_instructions(code, device->program_load_cursor);
    }

    load_bytes_to_address(device, device->program_load_cursor, code.data(), code.size());

    device->pc = device->program_load_cursor;
    device->program_load_cursor += code.size();
  }

  instruction step_device_one_instruction(other_command_device* device) {
    return detail::read_current_instruction(device);
  }

  void end_device_cycle(other_command_device* device) {
    detail::update_timers(device);
  }

  uint32_t opcode_set_value_with_mask(uint32_t dest, uint32_t value, uint32_t mask, uint8_t shift) {
    return (dest & ~mask) | ((value << shift) & mask);
  }

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category) {
    return opcode_set_value_with_mask(opcode, category, instruction::kCategoryMask, instruction::kCategoryShift);
  }

  uint32_t opcode_set_type(uint32_t opcode, uint8_t type) {
    return opcode_set_value_with_mask(opcode, type, instruction::kTypeMask, instruction::kTypeShift);
  }

  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x) {
    return opcode_set_value_with_mask(opcode, x, instruction::kXRegisterMask, instruction::kXRegisterShift);
  }

  namespace detail {

    uint32_t set_upper_halfword(uint32_t value, uint16_t upper) {
      return (value & instruction::kLowerHalfwordMask) | (static_cast<uint32_t>(upper) << 16);
    }

    uint32_t set_lower_halfword(uint32_t value, uint16_t lower) {
      return (value & instruction::kUpperHalfwordMask) | static_cast<uint32_t>(lower);
    }

  }  // namespace detail

  uint32_t opcode_set_upper(uint32_t opcode, uint16_t upper) {
    return detail::set_upper_halfword(opcode, upper);
  }

  uint32_t opcode_set_lower(uint32_t opcode, uint16_t lower) {
    return detail::set_lower_halfword(opcode, lower);
  }

  namespace detail {

    uint8_t get_category_nibble(uint32_t opcode) {
      return static_cast<uint8_t>((opcode & instruction::kCategoryMask) >> instruction::kCategoryShift);
    }

    uint8_t get_type_nibble(uint32_t opcode) {
      return static_cast<uint8_t>((opcode & instruction::kTypeMask) >> instruction::kTypeShift);
    }

    uint32_t get_group0_category() {
      return opcode_set_category(0x0, 0x0);
    }

    uint32_t group0_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group0_category(), type);
    }

    uint32_t group0_opcode_with_register(uint32_t opcode, uint8_t type, uint8_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group0_category(), type), reg);
    }

    uint32_t get_group1_category() {
      return opcode_set_category(0x0, 0x1);
    }

    uint32_t group1_opcode(uint32_t opcode, uint8_t type, uint8_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group1_category(), type), reg), addr);
    }

    uint32_t group1_opcode_with_constant_and_address(uint32_t opcode, uint8_t type, uint8_t context_idx, uint16_t addr) {
      opcode = opcode_set_type(get_group1_category(), type);
      opcode = opcode_set_value_with_mask(opcode, context_idx, instruction::kXRegisterMask, instruction::kXRegisterShift);
      return opcode_set_lower(opcode, addr);
    }

    uint32_t get_group2_category() {
      return opcode_set_category(0x0, 0x2);
    }

    uint32_t group2_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group2_category(), type);
    }

    uint32_t group2_opcode_with_register(uint32_t opcode, uint8_t type, uint16_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg);
    }

    uint32_t group2_opcode_with_address(uint32_t opcode, uint8_t type, uint16_t addr) {
      return opcode_set_lower(opcode_set_type(get_group2_category(), type), addr);
    }

    uint32_t group2_opcode_with_register_and_address(uint32_t opcode, uint8_t type, uint16_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg), addr);
    }

    uint32_t get_group3_category() {
      return opcode_set_category(0x0, 0x3);
    }

    uint32_t group3_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group3_category(), type);
    }

    uint32_t group3_opcode_with_address(uint32_t opcode, uint8_t type, uint16_t addr) {
      return opcode_set_lower(opcode_set_type(get_group3_category(), type), addr);
    }

  }  // namespace detail

  /// group 0 start ---------------
  uint32_t opcode_stop_device() {
    return 0x00000000;
  }

  uint32_t opcode_dump_registers() {
    return detail::group0_opcode(0x0, 0x1);
  }

  uint32_t opcode_dump_register_x(uint8_t x) {
    return detail::group0_opcode_with_register(0x0, 0x2, x);
  }
  /// group 0 end -----------------

  /// group 1 start ---------------
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x0, x, n);
  }

  uint32_t opcode_load_x_from(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x1, x, n);
  }

  uint32_t opcode_load_x_direct(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x2, x, n);
  }

  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x) {
    return detail::group1_opcode(0x0, 0x3, x, n);
  }

  uint32_t opcode_write_x_to_environment_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x4, x, n);
  }

  uint32_t opcode_load_x_from_environment_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x5, x, n);
  }
  /// group 1 end -----------------

  /// group 2 start ---------------
  uint32_t opcode_goto(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x0, n);
  }

  uint32_t opcode_goto_if_zero(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x1, n);
  }

  uint32_t opcode_goto_if_x_zero(uint32_t x, uint64_t n) {
    return detail::group2_opcode_with_register_and_address(0x0, 0x2, x, n);
  }

  uint32_t opcode_call_at(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x3, n);
  }

  uint32_t opcode_return() {
    return detail::group2_opcode(0x0, 0x4);
  }

  uint32_t opcode_return_value_in_x(uint32_t x) {
    return detail::group2_opcode_with_register(0x0, 0x5, x);
  }
  /// group 2 end -----------------

  /// group 3 start ---------------
  uint32_t opcode_execute_function_at(uint16_t n) {
    return detail::group3_opcode_with_address(0x0, 0x0, n);
  }
  /// group 3 end -----------------

  namespace detail {

    /////////////////////// 0XXX /////////////////////
    /// 00000000 - Stop the device
    void execute_stop_device(other_command_device* device, execution_context* exec) {
      device->stopped = true;
    }

    /// 01000000 - Dump all registers
    void execute_dump_registers(other_command_device* device, execution_context* exec) {
      for (size_t i = 0; i < other_command_device::kNumRegisters; ++i) {
        CORE_LOG_DEBUG("R[{}] = {:#018x}", i, device->registers[i].to_ullong());
      }
    }

    /// 02xx0000 - Dump register x
    void execute_dump_register_x(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      assert(x < other_command_device::kNumRegisters && "Register out of bounds!");
      CORE_LOG_DEBUG("R[{}] = {:#018x}", x, device->registers[x].to_ullong());
    }

    /////////////////////// 1XXX /////////////////////
    /// 10xxnnnn MEM[n] = R[x]
    void execute_write_x_to_memory(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint64_t addr = device->current_instruction.lower;
      device->write_u64_at(addr, device->registers[x].to_ullong());
    }

    /// 11xxnnnn R[x] = MEM[n]
    void execute_load_x_from_memory(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      device->registers[x] = device->read_u64_at(addr);
    }

    /// 12xxnnnn - R[x] = value
    void execute_load_x_direct(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t value = device->current_instruction.lower;
      device->registers[x] = value;
    }

    /// 13xxnnnn - MEM[n] = MEM[R[x]]
    void execute_indirect_write_x_to_memory(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      uint64_t value = device->read_u64_at(device->registers[x].to_ullong());
      device->write_u64_at(addr, value);
    }

    /// 14xxnnnn - ENV_MEM[n] = R[x]
    void execute_write_x_to_environment_memory(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;

      /// in this ctx addr points to execution context heap memory
      exec->memory[addr] = device->registers[x].to_ullong();
    }

    /// 15xxnnnn - R[x] = ENV_MEM[n]
    void execute_load_x_from_environment_memory(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;

      /// in this ctx addr points to execution context heap memory
      switch (exec->memory[addr].type()) {
        case value_type::INT8: device->registers[x] = (int8_t)exec->memory[addr]; break;
        case value_type::INT16: device->registers[x] = (int16_t)exec->memory[addr]; break;
        case value_type::INT32: device->registers[x] = (int32_t)exec->memory[addr]; break;
        case value_type::INT64: device->registers[x] = (int64_t)exec->memory[addr]; break;
        case value_type::UINT8: device->registers[x] = (uint8_t)exec->memory[addr]; break;
        case value_type::UINT16: device->registers[x] = (uint16_t)exec->memory[addr]; break;
        case value_type::UINT32: device->registers[x] = (uint32_t)exec->memory[addr]; break;
        case value_type::UINT64: device->registers[x] = (uint64_t)exec->memory[addr]; break;
        default:
          OTHER_ASSERT(false, "Unsupported value type {} for load_x_from_environment_memory", exec->memory[addr].type());
      }
    }

    /////////////////////// 2XXX /////////////////////
    /// 2000nnnn - goto address nnn
    void execute_goto(other_command_device* device, execution_context* exec) {
      uint16_t addr = device->current_instruction.lower;
      device->pc = addr;
      CORE_LOG_DEBUG("GOTO [{:#06x}]", addr);
    }

    /// 2100nnnn - goto address nnn if R[flag] == 0
    void execute_goto_if_zero(other_command_device* device, execution_context* exec) {
      uint16_t addr = device->current_instruction.lower;
      CORE_LOG_DEBUG("GOTO-IF-ZERO [{:#06x}]", addr);
      if (device->registers[other_command_device::kFlagRegister].to_ullong() == 0) {
        CORE_LOG_DEBUG("  - TAKING BRANCH");
        device->pc = addr;
      }
    }

    /// 22xxnnnn - goto address nnn if R[x] == 0
    void execute_goto_if_x_zero(other_command_device* device, execution_context* exec) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      CORE_LOG_DEBUG("GOTO-IF-X-ZERO R[{}] == 0 [{:#06x}]", x, addr);
      if (device->registers[x].to_ullong() == 0) {
        CORE_LOG_DEBUG("  - TAKING BRANCH");
        device->pc = addr;
      }
    }

    /// 2300nnnn - call function at address nnn
    void execute_call_at(other_command_device* device, execution_context* exec) {
      uint16_t addr = device->current_instruction.lower;

      // push current pc to stack
      device->stack[device->sp] = device->pc;
      device->sp++;
      device->pc = addr;
    }

    /// 24000000 - return from function
    void execute_return(other_command_device* device, execution_context* exec) {
      if (device->sp == 0) {
        assert(false && "Stack underflow on RET");
      } else {
        device->sp--;
        device->pc = device->stack[device->sp];
        device->stack[device->sp] = 0;
      }
    }

    //////////////////////// 3xxx /////////////////////
    /// 3000nnnn - execute function at address nnn
    void execute_execute_function_at(other_command_device* device, execution_context* exec) {
      uint16_t addr = device->current_instruction.lower;
      exec->functions.at(addr).exec(exec);
    }

    constexpr handler kDeviceControlTable[] = {
      execute_stop_device,
      execute_dump_registers,
      execute_dump_register_x,
    };
    constexpr handler kLoadTable[] = {
      execute_write_x_to_memory,
      execute_load_x_from_memory,
      execute_load_x_direct,
      execute_indirect_write_x_to_memory,
      execute_write_x_to_environment_memory,
      execute_load_x_from_environment_memory,
    };
    constexpr handler kProgramFlowTable[] = {
      execute_goto,
      execute_goto_if_zero,
      execute_goto_if_x_zero,
      execute_call_at,
      execute_return,
    };
    constexpr handler kEnvironmentTable[] = {
      execute_execute_function_at,
    };

    void execut_device_control_instruction(other_command_device* device, execution_context* exec) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      kDeviceControlTable[func_nib](device, exec);
    }

    void execute_load(other_command_device* device, execution_context* exec) {
      uint8_t load_nib = device->current_instruction.type_nibble();
      return kLoadTable[load_nib](device, exec);
    }

    void execute_program_flow(other_command_device* device, execution_context* exec) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      return kProgramFlowTable[func_nib](device, exec);
    }

    void execute_environment_instruction(other_command_device* device, execution_context* exec) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      return kEnvironmentTable[func_nib](device, exec);
    }

    constexpr handler kControlTable[] = {
      execut_device_control_instruction,
      execute_load,
      execute_program_flow,
      execute_environment_instruction,
    };

    handler* get_builtin_control_table_handler(control_tables table) {
      switch (table) {
        case CONTROL_TABLE_V000: return (handler*)kControlTable;
        default:
          OTHER_ASSERT(false, "Invalid control table: {}!", table);
      }
    }

  };  // namespace detail

  void program::start_function(const std::string_view name) {
    current_function = &functions.emplace_back(function{ .name = std::string{ name } });
    current_function->address = current_offset;
  }

  void program::end_function() {
    assert(current_function && "Current Function is null!");
    add_opcode(opcode_return());
    current_function = nullptr;
  }

  void program::dump_x(uint8_t x) {
    add_opcode(opcode_dump_register_x(x));
  }

  void program::write_x_to_memory(uint8_t x, uint16_t n) {
    add_opcode(opcode_write_x_to_memory(x, n));
  }

  void program::load_x_from(uint8_t x, uint16_t n) {
    add_opcode(opcode_load_x_from(x, n));
  }

  void program::load_x_direct(uint8_t x, uint16_t n) {
    add_opcode(opcode_load_x_direct(x, n));
  }

  void program::indirect_write_to(uint16_t n, uint8_t x) {
    add_opcode(opcode_indirect_write_x_to_memory(n, x));
  }

  void program::write_x_to_environment(uint8_t x, uint16_t n) {
    add_opcode(opcode_write_x_to_environment_memory(x, n));
  }

  void program::load_x_from_environment(uint8_t x, uint16_t n) {
    add_opcode(opcode_load_x_from_environment_memory(x, n));
  }

  void program::call(const std::string_view name) {
    assert(current_function && "Current Function is null!");
    calls.emplace_back(call_instruction{
      .code_offset = static_cast<uint16_t>(current_function->code.size()),
      .from_address = current_function->address,
      .name = std::string{ name },
    });
    /// save an opcode (uint32_t) worth of 0xFF bytes as a placeholder for the call instruction
    uint8_t byte = 0xFF;
    for (uint8_t count = 0; count < sizeof(uint32_t); ++count) {
      current_function->code.emplace_back(byte);
      ++current_offset;
    }
  }

  void program::execute_function_at(uint16_t n) {
    add_opcode(opcode_execute_function_at(n));
  }

  void program::dump_program() const {
    for (const auto& func : functions) {
      CORE_LOG_DEBUG("Function [{}] at address [{:#06x}] with {} bytes of code", func.name, func.address, func.code.size());
      dump_instructions(func.code, func.address);
    }
  }

  std::vector<uint8_t> program::compile_program(const uint64_t start_address) {
    std::vector<uint8_t> result;

    /// call main function (offset will be after this call instruction, then stop device so 2 + 2)
    uint32_t opcode = other::opcode_call_at(start_address + kDeviceProgramStartCodeOffset);
    uint8_t* opcode_bytes = reinterpret_cast<uint8_t*>(&opcode);

    uint32_t stop_opcode = other::opcode_stop_device();
    uint8_t* stop_opcode_bytes = reinterpret_cast<uint8_t*>(&stop_opcode);

    auto opcode_buffer = std::span(opcode_bytes, sizeof(uint32_t));
    result.append_range(opcode_buffer);

    auto stop_opcode_buffer = std::span(stop_opcode_bytes, sizeof(uint32_t));
    result.append_range(stop_opcode_buffer);

    uint16_t compiled_offset = start_address + kDeviceProgramStartCodeOffset;
    for (auto& func : functions) {
      func.compiled_address = compiled_offset;
      compiled_offset += func.code.size();
    }

    for (const auto& call : calls) {
      auto callee_fn_itr = std::ranges::find(functions, call.from_address, &function::address);
      assert(callee_fn_itr != functions.end() && "Invalid function call! Undefined function!");

      std::string name = call.name;
      auto function_to_call = std::ranges::find(functions, name, &function::name);
      assert(function_to_call != functions.end() && "Invalid function call! Undefined function!");

      uint16_t call_address = function_to_call->compiled_address;
      CORE_LOG_DEBUG(" - [from '{}'] call to [{}] at offset [{:#06x}] pointed to [{}] at address [{:#06x}]", callee_fn_itr->name, function_to_call->name, call.code_offset, function_to_call->name, call_address);

      uint16_t addr = function_to_call->compiled_address;
      uint32_t opcode = other::opcode_call_at(addr);
      *reinterpret_cast<uint32_t*>(&callee_fn_itr->code[call.code_offset]) = opcode;
    }

    for (auto& func : functions) {
      result.append_range(func.code);
    }

    CORE_LOG_DEBUG("Compiled program with {} instructions, size = {} bytes", num_instructions + 2u, result.size());
    return result;
  }

  void program::add_opcode(uint32_t opcode) {
    assert(current_function && "Current Function is null!");

    uint8_t* bytes = reinterpret_cast<uint8_t*>(&opcode);
    for (const auto& byte : std::span(bytes, sizeof(uint32_t))) {
      current_function->code.emplace_back(byte);
      current_offset++;
    }
    ++num_instructions;
  }

}  // namespace other