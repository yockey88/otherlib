/**
 * \file driver/systems/event_driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP

#include "core/scope.hpp"
#include "core/value.hpp"
#include "event/event_system.hpp"
#include "file/file_watcher.hpp"

#include "driver/systems/core_system.hpp"

namespace other {

  class OTHER_CLASS event_driver_system : public core_system<event_driver_system> {
   public:
    event_driver_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::EVENT_DRIVER_SYSTEM) {}
    ~event_driver_system() override = default;

    std::string name() const override { return "Event Driver System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    scope<event_system>& events();
    const scope<event_system>& events() const;

    void trigger_event(driver_kernel* kernel, const std::string_view name, const value& data = {});

    void handle_open_ui_window_event(driver_kernel* kernel, const value& data);
    void handle_close_ui_window_event(driver_kernel* kernel, const value& data);

   private:
    scope<event_system> event_system_ptr = nullptr;

    // void handle_object_driver_create_event(driver_kernel* kernel, const value& data);
    // void handle_object_driver_destroy_event(driver_kernel* kernel, const value& data);
    // void handle_object_driver_push_event(driver_kernel* kernel, const value& data);
    // void handle_object_driver_pop_event(driver_kernel* kernel, const value& data);
    // void handle_object_driver_info_event(driver_kernel* kernel, const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP