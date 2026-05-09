/**
 * \file driver/systems/vm_systems.cpp
 **/
#include "driver/systems/vm_system.hpp"

#include "vm/vm.hpp"

namespace other {

  void vm_system::initialize(driver_kernel* kernel) {
    vm::initialize_device(&core_device);
    vm::activate_builtin_control_table(&core_device, OTHER_CONTROL_TABLE_V000);
    core_device.host_driver = &get_driver();
  }

  void vm_system::tick(driver_kernel* kernel, double dt) {
    driver_step_device();
  }

  void vm_system::shutdown(driver_kernel* kernel) {
    core_device.stopped = true;
    vm::shutdown_device(&core_device);
  }

  void vm_system::write_id_at_address(uint16_t address, natural_t id) {
    OTHER_ASSERT(address + sizeof(natural_t) <= other_command_device::kMemorySize, "Address out of bounds: {}", address);
    core_device.write_u64_at(address, id);
  }

  void vm_system::emit_instruction(const instruction& op) {
    if (core_device.stopped) {
      core_device.stopped = false;
    }

    auto data = std::span(reinterpret_cast<const uint8_t*>(&op.opcode), sizeof(op.opcode));
    vm::load_bytes_to_address(&core_device, core_device.program_load_cursor, data.data(), data.size());
    core_device.program_load_cursor += data.size();
  }

  void vm_system::execute_driver_command(const std::string& command) {
  }

  void vm_system::driver_step_device() {
    PROFILE_SECTION("vm_system::driver_step_device");
    if (core_device.stopped) {
      return;
    }

    core_device.current_instruction = *(uint32_t*)&core_device.memory->at(core_device.pc);
    core_device.pc += other_command_device::kOpCodeSize;

    uint8_t instr_nib = core_device.current_instruction.category_nibble();
    core_device.control_table[instr_nib](&core_device);
    vm::update_device_timers(&core_device);
  }

}  // namespace other