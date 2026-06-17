/**
 * \file vm/vm_execution_context.hpp
 **/
#ifndef OTHERLIB_VM_VM_EXECUTION_CONTEXT_HPP
#define OTHERLIB_VM_VM_EXECUTION_CONTEXT_HPP

#include "vm/other_device.hpp"

namespace other {

  struct vm_execution_context {
    uint16_t pc = 0;
    uint8_t sp = 0;
    uint16_t stack[other_command_device::kStackSize] = {};
    vm_register registers[vm_register::kNumRegisters + 1] = {};
    uint64_t state_flags = 0;  // STOPPED / VM_ERROR / PAUSED snapshot
    const other_command_table* control_table = nullptr;
    other_command_device::program_metadata metadata = {};
    // Phase 3 adds: const import-table pointer. Phase 5 adds: hook_context_packed.

    bool live = false;
    bool stopped = true;

    void save_from(const other_command_device* device);
    void restore_to(other_command_device* device) const;
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_EXECUTION_CONTEXT_HPP