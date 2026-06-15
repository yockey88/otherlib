/**
 * \file driver/systems/vm_systems.cpp
 **/
#include "driver/systems/vm_system.hpp"

#include "driver/driver.hpp"
#include "vm/devices/core_command_device.hpp"
#include "vm/vm.hpp"

namespace other {

  void vm_system::initialize(driver_kernel* kernel) {
    core_device.host_driver = &get_driver();
    vm::initialize_device(&core_device);
    vm::load_control_table(&core_device, OTHER_CONTROL_TABLE_V000);

    OTHER_ASSERT(core_device.bus != nullptr, "Core device bus is null!");
    core_device.bus->register_device(make_scope<core_command_device>());

    vm::set_debug_mode(get_driver().get_config_value<bool>("driver.vm-debug-mode-on", false));
    instruction_budget = get_driver().get_config_value<uint32_t>("driver.vm-instruction-per-step-budget", kDefaultInstructionBudget);
  }

  void vm_system::tick(driver_kernel* kernel, double dt) {
    if (!boot_loaded && get_driver().current_driver_state() == driver_state::DRIVER_STATE_RUNNING) {
      if (std::string boot_oasm_path = get_driver().get_config_value<std::string>("driver.boot-file"); !boot_oasm_path.empty()) {
        CORE_LOG_INFO("Loading VM boot file: {}", boot_oasm_path);
        vm::load_program_from_file(&core_device, boot_oasm_path);
      }
      boot_loaded = true;
    }

    if (vm::has_flag(&core_device, other_command_device::DEBUG)) {
      return;
    }

    uint32_t executed = 0;
    while (executed < instruction_budget && !core_device.stopped &&
           !vm::has_flag(&core_device, other_command_device::STOPPED)) {
      vm::step(&core_device);
      ++executed;

      if (vm::has_flag(&core_device, other_command_device::VM_ERROR)) {
        CORE_LOG_ERROR("VM encountered an error");
        break;
      }
    }
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
    vm::load_program_from_bytes(&core_device, data);
    core_device.program_load_cursor += data.size();
  }

  void vm_system::execute_driver_command(const std::string& command) {
  }

}  // namespace other