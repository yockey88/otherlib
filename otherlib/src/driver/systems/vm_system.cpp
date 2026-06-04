/**
 * \file driver/systems/vm_systems.cpp
 **/
#include "driver/systems/vm_system.hpp"

#include "driver/driver.hpp"
#include "vm/vm.hpp"

namespace other {

  void vm_system::initialize(driver_kernel* kernel) {
    vm::initialize_device(&core_device);
    vm::load_control_table(&core_device, OTHER_CONTROL_TABLE_V000);
    core_device.host_driver = &get_driver();

    const bool vm_debug = get_driver().get_config_value<bool>("driver.debug-vm", false);
    vm::set_debug_mode(vm_debug);

    if (std::string boot_oasm_path = get_driver().get_config_value<std::string>("driver.boot-file"); !boot_oasm_path.empty()) {
      CORE_LOG_INFO("Loading VM boot file: {}", boot_oasm_path);
      vm::load_program_from_file(&core_device, boot_oasm_path);
    } else {
      CORE_LOG_INFO("No VM boot file specified in configuration. VM will start with empty memory.");
    }
  }

  void vm_system::tick(driver_kernel* kernel, double dt) {
    if (vm::has_flag(&core_device, other_command_device::DEBUG)) {
      return;
    }
    vm::step(&core_device);
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