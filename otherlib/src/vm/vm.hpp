/**
 * \file vm/vm.hpp
 **/
#ifndef OTHERLIB_VM_VM_HPP
#define OTHERLIB_VM_VM_HPP

#include "vm/other_device.hpp"
#include "vm/vm_version.hpp"

#include "control_table.hpp"

namespace other {

  struct other_command_device;

  struct vm {
    static void initialize_device(other_command_device* device);
    static void shutdown_device(other_command_device* device);
    static void update_device_timers(other_command_device* device);

    static void step(other_command_device* device);

    static void activate_builtin_control_table(other_command_device* device, control_tables table);

    static void load_bytes_to_address(other_command_device* device, uint64_t address, const uint8_t* data, size_t size);
    static void load_program_from_bytes(other_command_device* device, const std::span<const uint8_t> bytes);

    static void write_instruction_at_address(other_command_device* device, uint64_t address, const instruction& instr);
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_HPP