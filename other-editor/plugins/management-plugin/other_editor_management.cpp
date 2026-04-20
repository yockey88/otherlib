/**
 * \file other-editor-management.cpp
 **/
#include "driver/driver_system.hpp"

#include "other.hpp"

class OTHER_CLASS other_editor_management_plugin : public other::driver_plugin {
 public:
  other_editor_management_plugin(other::driver* driver_instance)
      : driver_plugin(driver_instance) {}

  std::string name() const override { return "Other Editor Management Plugin"; }

  void on_initialize() override {
    CORE_LOG_INFO("Initialized Other Editor Management Plugin.");
  }
  void on_tick(double dt) override {
    // CORE_LOG_DEBUG("Other Editor Management Plugin tick: {} ms", dt * 1000.0);
  }
  void on_shutdown() override {
    CORE_LOG_INFO("Shutting down Other Editor Management Plugin.");
  }
};

OTHER_PLUGIN(other_editor_management_plugin)