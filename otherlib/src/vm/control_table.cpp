/**
 * \file vm/control_table.cpp
 **/
#include "vm/control_table.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "vm/driver_interface.hpp"
#include "vm/other_device.hpp"

namespace other {

  void perform_call_stack_push_and_address_shift(other_command_device* device, uint16_t address) {
    device->stack[device->sp] = device->pc;
    device->sp++;
    device->pc = address;
  }

  void perform_call_stack_pop_and_address_shift(other_command_device* device) {
    device->sp--;
    device->pc = device->stack[device->sp];
    device->stack[device->sp] = 0;
  }

  namespace v000 {

    void execute_device_control_instruction(other_command_device* device);
    void execute_register_control(other_command_device* device);
    void execute_program_flow(other_command_device* device);
    void execute_arithmetic_logic(other_command_device* device);
    void execute_core_scene_control(other_command_device* device);

    void execute_debugger_control_instruction(other_command_device* device);
    void execute_debugger_register_control(other_command_device* device);
    void execute_debugger_program_flow(other_command_device* device);
    void execute_debugger_arithmetic_logic(other_command_device* device);
    void execute_debugger_core_scene_control(other_command_device* device);

    void execute_decompiler_control_instruction(other_command_device* device);
    void execute_decompiler_register_control(other_command_device* device);
    void execute_decompiler_program_flow(other_command_device* device);
    void execute_decompiler_arithmetic_logic(other_command_device* device);
    void execute_decompiler_core_scene_control(other_command_device* device);

    other_command_executor kControlTable[] = {
      execute_device_control_instruction,
      execute_register_control,
      execute_program_flow,
      execute_arithmetic_logic,
      execute_core_scene_control,
    };

    other_command_executor kDebuggerTable[] = {
      execute_debugger_control_instruction,
      execute_debugger_register_control,
      execute_debugger_program_flow,
      execute_debugger_arithmetic_logic,
      execute_debugger_core_scene_control,
    };

    other_command_executor kDecompilerTable[] = {
      execute_decompiler_control_instruction,
      execute_decompiler_register_control,
      execute_decompiler_program_flow,
      execute_decompiler_arithmetic_logic,
      execute_decompiler_core_scene_control,
    };

  }  // namespace v000

  void load_builtin_control_table(other_command_device* device, control_tables table) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    switch (table) {
      case OTHER_CONTROL_TABLE_V000: device->control_table = (other_command_executor*)v000::kControlTable; break;
      case OTHER_CONTROL_TABLE_DEBUGGER_V000: device->control_table = (other_command_executor*)v000::kDebuggerTable; break;
      case OTHER_CONTROL_TABLE_DECOMPILER_V000: device->control_table = (other_command_executor*)v000::kDecompilerTable; break;
      default:
        OTHER_ASSERT(false, "Invalid control table: {}!", table);
    }
  }

  namespace v000 {

    /////////////////////// 0XXX /////////////////////
    /// 00000000 - Stop the device
    void execute_stop_device(other_command_device* device) {
      device->stopped = true;
    }

    /// 01000000 - Dump all registers
    void execute_dump_registers(other_command_device* device) {
      for (size_t i = 0; i < other_command_device::kNumRegisters; ++i) {
        CORE_LOG_DEBUG("R[{}] = {:#018x}", i, device->registers[i].to_u64());
      }
      CORE_LOG_DEBUG("R[FLAG] = {:#018x}", device->registers[other_command_device::kFlagRegister].to_u64());
    }

    /// 02xx0000 - Dump register x
    void execute_dump_register_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      OTHER_ASSERT(x < other_command_device::kNumRegisters, "Register out of bounds!");
      CORE_LOG_DEBUG("R[{}] = {:#018x}", x, device->registers[x].to_u64());
    }

    /// 03xxnnnn - Dump memory at address n
    void execute_dump_memory_at(other_command_device* device) {
      // dump range [n, n + reg(x)] in a hexdump format
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      natural_t val_in_x = device->registers[x].to_u64();

      uint16_t n = device->current_instruction.lower;
      void* ptr = device->memory->ptr_to(n + device->program_start_address);
      size_t bytes_to_dump = std::min<size_t>(val_in_x, 256);  // limit to 256 bytes
      const uint8_t* byte_ptr = static_cast<const uint8_t*>(ptr);
      std::stringstream ss;
      ss << std::format("Memory dump at address {:#06x} ({} bytes):\n", n, bytes_to_dump);
      for (size_t i = 0; i < bytes_to_dump; ++i) {
        if (i % 16 == 0) {
          ss << std::format("{:#06x}: ", n + i);
        }
        ss << std::format("{:02x} ", byte_ptr[i]);
        if (i % 16 == 15 || i == bytes_to_dump - 1) {
          ss << "\n";
        }
      }
      CORE_LOG_DEBUG("MEM[{:#06x}] length = {}:\n{}", n, bytes_to_dump, ss.str());
    }

    /////////////////////// 1XXX /////////////////////
    /// 10xxnnnn MEM[n] = R[x]
    void execute_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint64_t addr = device->current_instruction.lower;
      device->write_u64_at(addr, device->registers[x].to_u64());
    }

    /// 11xxnnnn R[x] = MEM[n]
    void execute_load_x_from_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      uint64_t value = device->read_u64_at(addr + device->program_start_address);
      device->registers[x] = uint64_t{ value };
    }

    /// 12xxkkkk - R[x] = value
    void execute_load_x_direct(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t value = device->current_instruction.lower;
      device->registers[x] = uint64_t{ value };
    }

    /// 13xxnnnn - MEM[n] = MEM[R[x]]
    void execute_indirect_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      uint64_t value_addr = device->registers[x].to_u64();
      uint64_t value = device->read_u64_at(value_addr);
      device->write_u64_at(addr + device->program_start_address, value);
    }

    /// 14xxyyzz - R[z] = R[x] == R[y]
    void execute_compare_x_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() == device->registers[y].to_u64()) };
    }

    /// 15xxyyzz - R[z] = R[x] > R[y]
    void execute_x_gt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() > device->registers[y].to_u64()) };
    }

    /// 16xxyyzz - R[z] = R[x] < R[y]
    void execute_x_lt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() < device->registers[y].to_u64()) };
    }

    /// 17xxyyzz - R[z] = R[x] & R[y]
    void execute_x_and_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() & device->registers[y].to_u64()) };
    }

    /// 18xxyyzz - R[z] = R[x] | R[y]
    void execute_x_or_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() | device->registers[y].to_u64()) };
    }

    /// 19xxyyzz - R[z] = R[x] ^ R[y]
    void execute_x_xor_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      device->registers[z] = uint64_t{ (device->registers[x].to_u64() ^ device->registers[y].to_u64()) };
    }

    /// 1Axxyy00 - R[x] = R[x] << R[y]
    void execute_shift_left_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      device->registers[x] = uint64_t{ (device->registers[x].to_u64() << device->registers[y].to_u64()) };
    }

    /// 1Bxxyy00 - R[x] = R[x] >> R[y]
    void execute_shift_right_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      device->registers[x] = uint64_t{ (device->registers[x].to_u64() >> device->registers[y].to_u64()) };
    }

    /////////////////////// 2XXX /////////////////////
    /// 2000nnnn - goto address nnn
    void execute_goto(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;
      device->pc = addr + device->program_start_address;
    }

    /// 2100nnnn - goto address nnn if R[x] == 0
    void execute_jump_if_zero(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      if (device->registers[other_command_device::kFlagRegister].to_u64() == 0) {
        device->pc = addr + device->program_start_address;
      }
    }

    /// 2200nnnn - goto address nnn if R[flag] != 0
    void execute_jump_if_not_zero(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      if (device->registers[other_command_device::kFlagRegister].to_u64() != 0) {
        device->pc = addr + device->program_start_address;
      }
    }

    /// 2300nnnn - call function at address nnn
    void execute_call_at(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      // push current pc to stack
      perform_call_stack_push_and_address_shift(device, addr + device->program_start_address);
    }

    /// 24000000 - return from function
    void execute_return(other_command_device* device) {
      if (device->sp == 0) {
        OTHER_ASSERT(false, "Stack underflow on RET, PC: {:#08x}", device->pc);
      } else {
        perform_call_stack_pop_and_address_shift(device);
      }
    }

    /// 25xx0000 - return value in R[x]
    void execute_return_value_in_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      if (device->sp == 0) {
        assert(false && "Stack underflow on RET");
      } else {
        perform_call_stack_pop_and_address_shift(device);
        device->registers[other_command_device::kReturnRegister] = device->registers[x];
      }
    }

    /////////////////////// 3XXX /////////////////////
    /// 30xy0000 - R[x] = R[x] + R[y]
    void execute_add_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      device->registers[x] = device->registers[x].to_u64() + device->registers[y].to_u64();
    }

    /// 31xy0000 - R[x] = R[x] - R[y]
    void execute_sub_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      device->registers[x] = device->registers[x].to_u64() - device->registers[y].to_u64();
    }

    /// 32xy0000 - R[x] = R[x] * R[y]
    void execute_mul_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      device->registers[x] = device->registers[x].to_u64() * device->registers[y].to_u64();
    }

    /// 33xy0000 - R[x] = R[x] / R[y]
    void execute_div_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];

      uint64_t divisor = device->registers[y].to_u64();
      if (divisor == 0) {
        CORE_LOG_ERROR("Division by zero in DIV R[{}] / R[{}]", x, y);
        device->registers[other_command_device::kFlagRegister] = 1;
      } else {
        device->registers[x] = device->registers[x].to_u64() / device->registers[y].to_u64();
      }
    }

    /// 34xy0000 - R[x] = R[x] % R[y]
    void execute_mod_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];

      uint64_t divisor = device->registers[y].to_u64();
      if (divisor == 0) {
        CORE_LOG_ERROR("Division by zero in MOD R[{}] %% R[{}]", x, y);
        device->registers[other_command_device::kFlagRegister] = 1;
      } else {
        device->registers[x] = device->registers[x].to_u64() % device->registers[y].to_u64();
      }
    }

    /////////////////////// 4XXX /////////////////////
    /// 4000nnnn - load scene with id nnn
    void execute_load_scene_with_id_at(other_command_device* device) {
      uint16_t n = device->current_instruction.lower;
      uint64_t scene_id = device->read_u64_at(n);
      driver_interface::set_scene_by_id(device->host_driver, scene_id);
    }

    constexpr other_command_executor kDeviceControlTable[] = {
      execute_stop_device,
      execute_dump_registers,
      execute_dump_register_x,
      execute_dump_memory_at,
    };
    constexpr other_command_executor kLoadTable[] = {
      execute_write_x_to_memory,
      execute_load_x_from_memory,
      execute_load_x_direct,
      execute_indirect_write_x_to_memory,
      execute_compare_x_y_set_z,
      execute_x_gt_y_set_z,
      execute_x_lt_y_set_z,
      execute_x_and_y_set_z,
      execute_x_or_y_set_z,
      execute_x_xor_y_set_z,
      execute_shift_left_x_by_y,
      execute_shift_right_x_by_y,
    };
    constexpr other_command_executor kProgramFlowTable[] = {
      execute_goto,
      execute_jump_if_zero,
      execute_jump_if_not_zero,
      execute_call_at,
      execute_return,
      execute_return_value_in_x,
    };
    constexpr other_command_executor kArithmeticLogicTable[] = {
      execute_add_x_y_to_x,
      execute_sub_x_y_to_x,
      execute_mul_x_y_to_x,
      execute_div_x_y_to_x,
      execute_mod_x_y_to_x,
    };
    constexpr other_command_executor kCoreSceneControlTable[] = {
      execute_load_scene_with_id_at,
    };

    void execute_device_control_instruction(other_command_device* device) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      kDeviceControlTable[func_nib](device);
    }

    void execute_register_control(other_command_device* device) {
      uint8_t load_nib = device->current_instruction.type_nibble();
      return kLoadTable[load_nib](device);
    }

    void execute_program_flow(other_command_device* device) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      return kProgramFlowTable[func_nib](device);
    }

    void execute_arithmetic_logic(other_command_device* device) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      return kArithmeticLogicTable[func_nib](device);
    }

    void execute_core_scene_control(other_command_device* device) {
      uint8_t func_nib = device->current_instruction.type_nibble();
      return kCoreSceneControlTable[func_nib](device);
    }

    /// \todo:
    /// debugger control tables
    void execute_debugger_control_instruction(other_command_device* device) {
    }

    void execute_debugger_register_control(other_command_device* device) {
    }

    void execute_debugger_program_flow(other_command_device* device) {
    }

    void execute_debugger_arithmetic_logic(other_command_device* device) {
    }

    void execute_debugger_core_scene_control(other_command_device* device) {
    }
    /// end debugger control tables

    void emit_instruction_log(other_command_device* device, const std::string_view msg) {
      const instruction& instr = device->current_instruction;

      std::stringstream ss;
      ss << std::format("[{:#06x}] : {:#010x}", device->pc, instr.opcode);
      ss << " " << msg;
      CORE_LOG_DEBUG("{}", ss.str());
    }

    /////////////////////// 0XXX /////////////////////
    /// 00000000 - Stop the device
    void execute_decompiler_stop_device(other_command_device* device) {
      emit_instruction_log(device, "[STOP-DEVICE]");
    }

    /// 01000000 - Dump all registers
    void execute_decompiler_dump_registers(other_command_device* device) {
      emit_instruction_log(device, "[DUMP-REGISTERS]");
    }

    /// 02xx0000 - Dump register x
    void execute_decompiler_dump_register_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[DUMP-REGISTER-X] x={}", x));
    }

    /// 03xxnnnn - Dump memory at address n
    void execute_decompiler_dump_memory_at(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t n = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[DUMP-MEMORY-AT] x={} n={:#06x}", x, n));
    }

    /////////////////////// 1XXX /////////////////////
    /// 10xxnnnn MEM[n] = R[x]
    void execute_decompiler_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint64_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[WRITE-X-TO-MEMORY] x={} n={:#06x}", x, addr));
    }

    /// 11xxnnnn R[x] = MEM[n]
    void execute_decompiler_load_x_from_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint64_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[LOAD-X-FROM-MEMORY] x={} n={:#06x}", x, addr));
    }

    /// 12xxnnnn - R[x] = value
    void execute_decompiler_load_x_direct(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t value = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[LOAD-X-DIRECT] x={} n={:#06x}", x, value));
    }

    /// 13xxnnnn - MEM[n] = MEM[R[x]]
    void execute_decompiler_indirect_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[INDIRECT-WRITE-X-TO-MEMORY] n={:#06x} x={}", addr, x));
    }

    /// 14xxyyzz - R[z] = R[x] == R[y]
    void execute_decompiler_compare_x_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[COMPARE-X-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 15xxyyzz - R[z] = R[x] > R[y]
    void execute_decompiler_x_gt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[X-GT-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 16xxyyzz - R[z] = R[x] < R[y]
    void execute_decompiler_x_lt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[X-LT-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 17xxyyzz - R[z] = R[x] & R[y]
    void execute_decompiler_x_and_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[X-AND-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 18xxyyzz - R[z] = R[x] | R[y]
    void execute_decompiler_x_or_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[X-OR-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 19xxyyzz - R[z] = R[x] ^ R[y]
    void execute_decompiler_x_xor_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[X-XOR-Y-SET-Z] x={} y={} z={}", x, y, z));
    }

    /// 1Axxyy00 - R[x] = R[x] << R[y]
    void execute_decompiler_shift_left_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[SHL R[{}] << R[{}] -> R[{}]]", x, y, x));
    }

    /// 1Bxxyy00 - R[x] = R[x] >> R[y]
    void execute_decompiler_shift_right_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[SHR R[{}] >> R[{}] -> R[{}]]", x, y, x));
    }

    /////////////////////// 2XXX /////////////////////
    /// 2000nnnn - goto address nnn
    void execute_decompiler_goto(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[GOTO] n={:#06x}", addr));
    }

    /// 2100nnnn - goto address nnn if R[flag] == 0
    void execute_decompiler_goto_if_zero(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[GOTO-IF-ZERO] n={:#06x}", addr));
    }

    /// 22xxnnnn - goto address nnn if R[x] == 0
    void execute_decompiler_goto_if_x_zero(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[GOTO-IF-X-ZERO] x={} n={:#06x}", x, addr));
    }

    /// 2300nnnn - call function at address nnn
    void execute_decompiler_call_at(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[CALL-AT] n={:#06x}", addr));
    }

    /// 24000000 - return from function
    void execute_decompiler_return(other_command_device* device) {
      emit_instruction_log(device, "[RETURN]");
    }

    ///////////////////////// 3XXX /////////////////////
    /// 30xy0000 - R[x] = R[x] + R[y]
    void execute_decompiler_add_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[ADD-X-Y-TO-X] x={} y={}", x, y));
    }

    /// 31xy0000 - R[x] = R[x] - R[y]
    void execute_decompiler_sub_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[SUB-X-Y-TO-X] x={} y={}", x, y));
    }

    /// 32xy0000 - R[x] = R[x] * R[y]
    void execute_decompiler_mul_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[MUL-X-Y-TO-X] x={} y={}", x, y));
    }

    /// 33xy0000 - R[x] = R[x] / R[y]
    void execute_decompiler_div_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[DIV-X-Y-TO-X] x={} y={}", x, y));
    }

    /// 34xy0000 - R[x] = R[x] % R[y]
    void execute_decompiler_mod_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      emit_instruction_log(device, std::format("[MOD-X-Y-TO-X] x={} y={}", x, y));
    }

    /////////////////////// 4XXX /////////////////////
    /// 4000nnnn - load scene with id nnn
    void execute_decompiler_load_scene_with_id_at(other_command_device* device) {
      uint16_t n = device->current_instruction.lower;
      emit_instruction_log(device, std::format("[LOAD-SCENE-WITH-ID-AT] n={}", n));
    }

    constexpr other_command_executor kDecompilerDeviceControlTable[] = {
      execute_decompiler_stop_device,
      execute_decompiler_dump_registers,
      execute_decompiler_dump_register_x,
      execute_decompiler_dump_memory_at,
    };
    constexpr other_command_executor kDecompilerLoadTable[] = {
      execute_decompiler_write_x_to_memory,
      execute_decompiler_load_x_from_memory,
      execute_decompiler_load_x_direct,
      execute_decompiler_indirect_write_x_to_memory,
      execute_decompiler_compare_x_y_set_z,
      execute_decompiler_x_gt_y_set_z,
      execute_decompiler_x_lt_y_set_z,
      execute_decompiler_x_and_y_set_z,
      execute_decompiler_x_or_y_set_z,
      execute_decompiler_x_xor_y_set_z,
      execute_decompiler_shift_left_x_by_y,
      execute_decompiler_shift_right_x_by_y,
    };
    constexpr other_command_executor kDecompilerProgramFlowTable[] = {
      execute_decompiler_goto,
      execute_decompiler_goto_if_zero,
      execute_decompiler_goto_if_x_zero,
      execute_decompiler_call_at,
      execute_decompiler_return,
    };
    constexpr other_command_executor kDecompilerArithmeticLogicTable[] = {
      execute_decompiler_add_x_y_to_x,
      execute_decompiler_sub_x_y_to_x,
      execute_decompiler_mul_x_y_to_x,
      execute_decompiler_div_x_y_to_x,
      execute_decompiler_mod_x_y_to_x,
    };
    constexpr other_command_executor kDecompilerCoreSceneControlTable[] = {
      execute_decompiler_load_scene_with_id_at,
    };

    void execute_decompiler_control_instruction(other_command_device* device) {
      uint8_t type = device->current_instruction.type_nibble();
      kDecompilerDeviceControlTable[type](device);
    }

    void execute_decompiler_register_control(other_command_device* device) {
      uint8_t type = device->current_instruction.type_nibble();
      kDecompilerLoadTable[type](device);
    }

    void execute_decompiler_program_flow(other_command_device* device) {
      uint8_t type = device->current_instruction.type_nibble();
      kDecompilerProgramFlowTable[type](device);
    }

    void execute_decompiler_arithmetic_logic(other_command_device* device) {
      uint8_t type = device->current_instruction.type_nibble();
      kDecompilerArithmeticLogicTable[type](device);
    }

    void execute_decompiler_core_scene_control(other_command_device* device) {
      uint8_t type = device->current_instruction.type_nibble();
      kDecompilerCoreSceneControlTable[type](device);
    }

  }  // namespace v000
}  // namespace other