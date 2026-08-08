/**
 * \file other-editor-management.cpp
 **/
#include "driver/driver_system.hpp"

#include "other.hpp"
// #include "tcp_listener.hpp"

class OTHER_CLASS other_editor_management_plugin : public other::driver_plugin {
 public:
  other_editor_management_plugin(other::driver* driver_instance)
      : driver_plugin(driver_instance) {}

  std::string name() const override { return "Other Editor Management Plugin"; }

  void on_initialize(other::driver_kernel& kernel) override {
    CORE_LOG_INFO("Initialized Other Editor Management Plugin.");

    // auto listener = other::make_scope<tcp_listener>();
    // kernel.get_core_system<other::network_system>().register_transport_listener("tcp", std::move(listener));
  }
  void on_tick(other::driver_kernel& kernel, double dt) override {
    // CORE_LOG_DEBUG("Other Editor Management Plugin tick: {} ms", dt * 1000.0);
  }
  void on_shutdown(other::driver_kernel& kernel) override {
    CORE_LOG_INFO("Shutting down Other Editor Management Plugin.");
  }
};

OTHER_PROVIDES(other_editor_management_plugin, other::driver_plugin, "other-editor");
OTHER_PLUGIN("Editor Management", "0.1.0", "N/A", "")