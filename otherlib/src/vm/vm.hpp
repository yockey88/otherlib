/**
 * \file vm/vm.hpp
 **/
#ifndef OTHERLIB_VM_VM_HPP
#define OTHERLIB_VM_VM_HPP

#include "vm/control_table.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/other_device.hpp"
#include "vm/vm_version.hpp"

namespace other {

  struct other_command_device;

  struct vm {
    enum state : uint8_t {
      STOPPED = 0,
      INITIALIZED = 1 << 0,

      IDLE = 1 << 1,
      RUNNING = 1 << 2,

      DEBUG = 1 << 3,
      REPL = 1 << 4,

      VM_ERROR = std::numeric_limits<uint8_t>::max(),
    };

    static other_command_device* get_initialized_device();
    static void set_debug_mode(bool enable);

    static bool has_flag(state flag);

    static void initialize_device(other_command_device* device);
    static void load_control_table(other_command_device* device, control_tables table);

    static void load_program_from_file(other_command_device* device, const filepath& file);
    static void load_program_from_bytes(other_command_device* device, const std::span<const uint8_t> bytes);
    static void step(other_command_device* device);

    static void shutdown_device(other_command_device* device);

    static natural_t get_register_as_u64(other_command_device* device, uint8_t reg_idx);
    static void write_register_from_u64(other_command_device* device, uint8_t reg_idx, uint64_t value);

    static void* register_memory(other_command_device* device, uint8_t reg_idx);
    static const void* register_memory(const other_command_device* device, uint8_t reg_idx);

    static void write_u64_at(other_command_device* device, uint64_t address, uint64_t value);
    static uint64_t read_u64_at(const other_command_device* device, uint64_t address);

    static void* memory_pointer(other_command_device* device, uint64_t address);
    static const void* memory_pointer(const other_command_device* device, uint64_t address);

   private:
    static diagnostic_engine diagnostics;
    static state current_mode;

    static void add_flag(state flag);
    static void remove_flag(state flag);

    static void update_device_timers(other_command_device* device);

    static void load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size);
    static void write_instruction_at_address(other_command_device* device, uint64_t address, const instruction& instr);
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_HPP