/**
 * \file driver/systems/driver_plugin.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_DRIVER_PLUGIN_HPP
#define OTHERLIB_DRIVER_SYSTEMS_DRIVER_PLUGIN_HPP

#include "core/interfaces.hpp"

#include "driver/driver_system.hpp"

namespace other {

  class OTHER_CLASS driver_plugin : public driver_system {
    OTHER_ENVIRONMENT_INTERFACE("Driver", "DriverPlugin", driver*);

   public:
    driver_plugin(driver* driver_instance)
        : driver_system(driver_instance, driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_END) {}
    virtual ~driver_plugin() override = default;

    bool active() const override { return is_active; }
    inline void set_active(bool is_active) { this->is_active = is_active; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    virtual void on_initialize(driver_kernel& kernel) {}
    virtual void on_tick(driver_kernel& kernel, double dt) {}
    virtual void on_shutdown(driver_kernel& kernel) {}

   private:
    bool is_active = false;
  };

  inline auto driver_plugin_args(driver* driver_instance) {
    return [driver_instance]() {
      return std::tuple<driver*>{ driver_instance };
    };
  }

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_DRIVER_PLUGIN_HPP