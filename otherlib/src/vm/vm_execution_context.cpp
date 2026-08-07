/**
 * \file vm/vm_execution_context.cpp
 **/
#include "vm/vm_execution_context.hpp"

#include "core/profiler.hpp"

namespace other {

  void vm_execution_context::save_from(const other_command_device* device) {
    OTHER_ASSERT(device != nullptr, "Null device!");
    PROFILE_SECTION("vm_execution_context::save_from");

    pc = device->pc;
    sp = device->sp;
    std::copy(std::begin(device->stack), std::end(device->stack), std::begin(stack));
    for (size_t i = 0; i < vm_register::kNumRegisters + 1; ++i) {
      registers[i] = device->registers[i];
    }
    state_flags = device->current_state;
    control_table = device->control_table;
  }

  void vm_execution_context::restore_to(other_command_device* device) const {
    OTHER_ASSERT(device != nullptr, "Null device!");
    PROFILE_SECTION("vm_execution_context::restore_to");

    device->pc = pc;
    device->sp = sp;
    std::copy(std::begin(stack), std::end(stack), std::begin(device->stack));
    for (size_t i = 0; i < vm_register::kNumRegisters + 1; ++i) {
      device->registers[i] = registers[i];
    }
    device->current_state = static_cast<other_command_device::state>(state_flags);
    device->control_table = control_table;
  }

}  // namespace other