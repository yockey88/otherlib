/**
 * \file driver/systems/vm_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_VM_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_VM_SYSTEM_HPP

#include <queue>

#include "driver/systems/core_system.hpp"
#include "vm/other_device.hpp"

namespace other {

  class OTHER_CLASS vm_system : public core_system<vm_system> {
   public:
    vm_system(driver* driver_instance)
        : core_system<vm_system>(driver_instance, static_cast<uint32_t>(driver_system_type::VM_DRIVER_SYSTEM)) {}

    std::string name() const override { return "VM System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    void write_id_at_address(uint16_t address, natural_t id);
    void emit_instruction(const instruction& op);
    void execute_driver_command(const std::string& command);

   private:
    other_command_device core_device;
    std::queue<instruction> emitted_instructions;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_VM_SYSTEM_HPP