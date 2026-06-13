/**
 * \file vm/control_table.cpp
 **/
#include "vm/control_table.hpp"

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "vm/command_bus.hpp"
#include "vm/driver_interface.hpp"
#include "vm/other_device.hpp"
#include "vm/vm.hpp"
#include "vm/vm_error.hpp"

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

  void execute_illegal_instruction_category(other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    CORE_LOG_ERROR("[VM] illegal category {:#x} at pc {:#06x} (opcode {:#010x})",
                   device->current_instruction.category_nibble(), device->pc,
                   device->current_instruction.opcode);
    device->write_flag_register(VM_ILLEGAL_INSTRUCTION_CATEGORY);
    vm::add_flag(device, other_command_device::VM_ERROR);
    device->stopped = true;
    CORE_LOG_ERROR("[VM] illegal category {:#x} at pc {:#06x} (opcode {:#010x})",
                   device->current_instruction.category_nibble(), device->pc,
                   device->current_instruction.opcode);
  }

  void execute_illegal_instruction(other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    CORE_LOG_ERROR("[VM] illegal instruction with opcode {:#010x} at pc {:#06x}",
                   device->current_instruction.opcode, device->pc);
    device->write_flag_register(VM_ILLEGAL_INSTRUCTION_TYPE);
    vm::add_flag(device, other_command_device::VM_ERROR);
    device->stopped = true;
    CORE_LOG_ERROR("[VM] illegal instruction with opcode {:#010x} at pc {:#06x}",
                   device->current_instruction.opcode, device->pc);
  }

  namespace v000 {

    void execute_device_control_instruction(other_command_device* device);
    void execute_register_control(other_command_device* device);
    void execute_program_flow(other_command_device* device);
    void execute_arithmetic_logic(other_command_device* device);
    constexpr other_command_table kControlTable = {
      execute_device_control_instruction,
      execute_register_control,
      execute_program_flow,
      execute_arithmetic_logic,
      // clang-format off
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,   execute_illegal_instruction_category, execute_illegal_instruction_category, execute_illegal_instruction_category,  // 4 - 9
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category, execute_illegal_instruction_category,  // A - F
      // clang-format on
    };

    void execute_debugger_control_instruction(other_command_device* device);
    void execute_debugger_register_control(other_command_device* device);
    void execute_debugger_program_flow(other_command_device* device);
    void execute_debugger_arithmetic_logic(other_command_device* device);
    constexpr other_command_table kDebuggerTable = {
      execute_debugger_control_instruction,
      execute_debugger_register_control,
      execute_debugger_program_flow,
      execute_debugger_arithmetic_logic,
      // clang-format off
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,   execute_illegal_instruction_category, execute_illegal_instruction_category, execute_illegal_instruction_category,  // 4 - 9
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category, execute_illegal_instruction_category,  // A - F
      // clang-format on
    };

    void execute_decompiler_control_instruction(other_command_device* device);
    void execute_decompiler_register_control(other_command_device* device);
    void execute_decompiler_program_flow(other_command_device* device);
    void execute_decompiler_arithmetic_logic(other_command_device* device);
    constexpr other_command_table kDecompilerTable = {
      execute_decompiler_control_instruction,
      execute_decompiler_register_control,
      execute_decompiler_program_flow,
      execute_decompiler_arithmetic_logic,
      // clang-format off
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,   execute_illegal_instruction_category, execute_illegal_instruction_category, execute_illegal_instruction_category,  // 4 - 9
      execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category,  execute_illegal_instruction_category, execute_illegal_instruction_category,  // A - F
      // clang-format on
    };

  }  // namespace v000

  void load_builtin_control_table(other_command_device* device, control_tables table) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    switch (table) {
      case OTHER_CONTROL_TABLE_V000: device->control_table = &v000::kControlTable; break;
      case OTHER_CONTROL_TABLE_DEBUGGER_V000: device->control_table = &v000::kDebuggerTable; break;
      case OTHER_CONTROL_TABLE_DECOMPILER_V000: device->control_table = &v000::kDecompilerTable; break;
      default:
        /// \todo: look up table in plugin registry or script system
        OTHER_ASSERT(false, "Invalid control table: {}!", table);
    }
  }

  namespace v000 {

    /////////////////////// 0XXX /////////////////////
    /// 00000000 - Stop the device
    void execute_stop_device(other_command_device* device) {
      device->stopped = true;
      vm::remove_flag(device, other_command_device::RUNNING);
      vm::remove_flag(device, other_command_device::IDLE);
      vm::add_flag(device, other_command_device::STOPPED);
    }

    /// 01000000 - Dump all registers
    void execute_dump_registers(other_command_device* device) {
      for (size_t i = 0; i < vm_register::kNumRegisters; ++i) {
        CORE_LOG_DEBUG("R[{}] = {:#018x}", i, device->registers[i].memory.to_u64());
      }
      CORE_LOG_DEBUG("R[FLAG] = {:#018x}", device->read_register_as_u64(vm_register_idx::VM_RFLAG));
    }

    /// 02xx0000 - Dump register x
    void execute_dump_register_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      OTHER_ASSERT(x < vm_register::kNumRegisters, "Register out of bounds!");
      CORE_LOG_INFO("R[{}] = {:#018x}", x, device->read_register_as_u64(x));
    }

    /// 03xxnnnn - Dump memory at address n
    void execute_dump_memory_at(other_command_device* device) {
      // dump range [n, n + reg(x)] in a hexdump format
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      natural_t val_in_x = device->read_register_as_u64(x);

      uint16_t n = device->current_instruction.lower;
      const void* ptr = device->access_current_program_memory(n);
      // limit to 256 bytes
      size_t bytes_to_dump = std::min<size_t>(val_in_x, 256);
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
      CORE_LOG_INFO("MEM[{:#06x}] length = {}:\n{}", n, bytes_to_dump, ss.str());
    }

    /// 04000000 - dumps the current execution context of the vm
    void execute_view_state(other_command_device* device) {
      std::stringstream ss;
      ss << "Execution context:\n";
      ss << std::format(" - Program counter: {:#06x}\n", device->pc);
      ss << std::format(" - Stack pointer: {:#06x}\n", device->sp);
      // ss << std::format(" - Frame pointer: {:#06x}\n", device->fp);
      ss << std::format(" - Registers:\n");
      for (size_t i = 0; i < vm_register::kNumRegisters; ++i) {
        ss << std::format("  - R[{}] = {:#018x}\n", i, device->read_register_as_u64(i));
      }
      ss << std::format(" - R[FLAG] = {:#018x}\n", device->read_flag_register());
      CORE_LOG_INFO("{}", ss.str());
    }

    /////////////////////// 1XXX /////////////////////
    /// 10xxnnnn MEM[n] = R[x]
    void execute_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint64_t addr = device->current_instruction.lower;
      device->write_u64_at(addr, device->read_register_as_u64(x));
    }

    /// 11xxnnnn R[x] = n
    void execute_load_x_from_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      device->write_register_from_u64(x, addr);
    }

    /// 12xxkkkk - R[x] = value
    void execute_load_x_direct(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t value = device->current_instruction.lower;
      device->write_register_from_u64(x, value);
    }

    /// 13xxnnnn - MEM[n] = MEM[R[x]]
    void execute_indirect_write_x_to_memory(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint16_t addr = device->current_instruction.lower;
      uint64_t value_addr = device->read_register_as_u64(x);
      uint64_t value = device->read_u64_at(value_addr);
      device->write_current_program_data_from_u64(addr, value);
      // write_u64_at(addr + device->program_start_address, value);
    }

    /// 14xxyyzz - R[z] = R[x] == R[y]
    void execute_compare_x_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];

      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x == val_y) });
    }

    /// 15xxyyzz - R[z] = R[x] > R[y]
    void execute_x_gt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x > val_y) });
    }

    /// 16xxyyzz - R[z] = R[x] < R[y]
    void execute_x_lt_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x < val_y) });
    }

    /// 17xxyyzz - R[z] = R[x] & R[y]
    void execute_x_and_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x & val_y) });
    }

    /// 18xxyyzz - R[z] = R[x] | R[y]
    void execute_x_or_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x | val_y) });
    }

    /// 19xxyyzz - R[z] = R[x] ^ R[y]
    void execute_x_xor_y_set_z(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      uint8_t z = device->current_instruction.bytes[instruction::Z_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(z, uint64_t{ (val_x ^ val_y) });
    }

    /// 1Axxyy00 - R[x] = R[x] << R[y]
    void execute_shift_left_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(x, uint64_t{ (val_x << val_y) });
    }

    /// 1Bxxyy00 - R[x] = R[x] >> R[y]
    void execute_shift_right_x_by_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(x, uint64_t{ (val_x >> val_y) });
    }

    /// 1Cxxyy00 - R[y] = R[x]
    void execute_move_x_to_y(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      device->write_register_from_u64(y, val_x);
    }

    /////////////////////// 2XXX /////////////////////
    /// 2000nnnn - goto address nnn
    void execute_goto(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;
      device->pc = device->globalize_address(addr);
    }

    /// 2100nnnn - goto address nnn if R[x] == 0
    void execute_jump_if_zero(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      natural_t flag = device->read_register_as_u64(vm_register_idx::VM_RFLAG);
      if (flag == 0) {
        device->pc = device->globalize_address(addr);
      }
    }

    /// 2200nnnn - goto address nnn if R[flag] != 0
    void execute_jump_if_not_zero(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      natural_t flag = device->read_register_as_u64(vm_register_idx::VM_RFLAG);
      if (flag != 0) {
        device->pc = device->globalize_address(addr);
      }
    }

    /// 2300nnnn - call function at address nnn
    void execute_call_at(other_command_device* device) {
      uint16_t addr = device->current_instruction.lower;

      // push current pc to stack
      perform_call_stack_push_and_address_shift(device, device->globalize_address(addr));
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
        device->write_register_from_u64(vm_register_idx::VM_RRETURN, device->read_register_as_u64(x));
      }
    }

    /// 2600kkkk - syscall with id kkkk
    void execute_syscall(other_command_device* device) {
      OTHER_ASSERT(device != nullptr, "Null device!");
      OTHER_ASSERT(device->bus != nullptr, "Device's command bus cannot be null for syscall execution");
      uint16_t id = device->current_instruction.lower;
      if (!device->bus->dispatch(id, device)) {
        device->write_flag_register(vm_error::VM_BAD_SYSCALL);
        device->write_register_from_u64(vm_register::kReturnRegister, 0);
        CORE_LOG_ERROR("[VM] Invalid syscall ID {:#06x} at PC {:#06x}", id, device->pc);
      }
    }

    /// 2700kkkk
    void execute_invoke(other_command_device* device) {
      OTHER_ASSERT(device != nullptr, "Null device!");
      OTHER_ASSERT(device->bus != nullptr, "Device's command bus cannot be null for invoke execution");
    }

    /////////////////////// 3XXX /////////////////////
    /// 30xy0000 - R[x] = R[x] + R[y]
    void execute_add_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(x, val_x + val_y);
    }

    /// 31xy0000 - R[x] = R[x] - R[y]
    void execute_sub_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(x, val_x - val_y);
    }

    /// 32xy0000 - R[x] = R[x] * R[y]
    void execute_mul_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];
      natural_t val_x = device->read_register_as_u64(x);
      natural_t val_y = device->read_register_as_u64(y);
      device->write_register_from_u64(x, val_x * val_y);
    }

    /// 33xy0000 - R[x] = R[x] / R[y]
    void execute_div_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];

      uint64_t divisor = device->read_register_as_u64(y);
      if (divisor == 0) {
        CORE_LOG_ERROR("Division by zero in DIV R[{}] / R[{}]", x, y);
        device->write_register_from_u64(vm_register_idx::VM_RFLAG, 1);
      } else {
        uint64_t dividend = device->read_register_as_u64(x);
        device->write_register_from_u64(x, dividend / divisor);
      }
    }

    /// 34xy0000 - R[x] = R[x] % R[y]
    void execute_mod_x_y_to_x(other_command_device* device) {
      uint8_t x = device->current_instruction.bytes[instruction::X_REGISTER_BYTE_IDX];
      uint8_t y = device->current_instruction.bytes[instruction::Y_REGISTER_BYTE_IDX];

      uint64_t divisor = device->read_register_as_u64(y);
      if (divisor == 0) {
        CORE_LOG_ERROR("Division by zero in MOD R[{}] %% R[{}]", x, y);
        device->write_register_from_u64(vm_register_idx::VM_RFLAG, 1);
      } else {
        uint64_t dividend = device->read_register_as_u64(x);
        device->write_register_from_u64(x, dividend % divisor);
      }
    }

    constexpr other_command_table kDeviceControlTable = {
      execute_stop_device,
      execute_dump_registers,
      execute_dump_register_x,
      execute_dump_memory_at,
      execute_view_state,
      // clang-format off
      execute_illegal_instruction, execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction, // 5 - 9
      execute_illegal_instruction, execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction, // A - F
      // clang-format on
    };
    constexpr other_command_table kLoadTable = {
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
      execute_move_x_to_y,
      // clang-format off
      execute_illegal_instruction, execute_illegal_instruction,  execute_illegal_instruction, // D - F
      // clang-format on
    };
    constexpr other_command_table kProgramFlowTable = {
      execute_goto,
      execute_jump_if_zero,
      execute_jump_if_not_zero,
      execute_call_at,
      execute_return,
      execute_return_value_in_x,
      execute_syscall,
      execute_invoke,
      // clang-format off
      execute_illegal_instruction,  execute_illegal_instruction, // 7 - 9
      execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction, // A - F
      // clang-format on
    };
    constexpr other_command_table kArithmeticLogicTable = {
      execute_add_x_y_to_x,
      execute_sub_x_y_to_x,
      execute_mul_x_y_to_x,
      execute_div_x_y_to_x,
      execute_mod_x_y_to_x,
      // clang-format off
      execute_illegal_instruction, execute_illegal_instruction, execute_illegal_instruction, execute_illegal_instruction, execute_illegal_instruction, // 5 - 9
      execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction,  execute_illegal_instruction, // A - F
      // clang-format on
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

    /// 2000nnnn (goto <label>/goto n)
    /// 2100nnnn (je <label>/je n)
    /// 2200nnnn (jne <label>/jne n)
    /// 23xxnnnn (call <label>/call n)
    /// 24000000 (ret)
    /// 25xx0000 (ret x)
    /// 2600kkkk (syscall x)

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

    /// 2500kkkk -

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

  }  // namespace v000
}  // namespace other