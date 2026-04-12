/**
 * \file driver/systems/event_driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP

#include "core/scope.hpp"
#include "core/value.hpp"
#include "event/event_system.hpp"

#include "driver/driver_kernel.hpp"

namespace other {

  class event_driver_system : public core_system<event_driver_system> {
   public:
    event_driver_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::EVENT_DRIVER_SYSTEM) {}
    ~event_driver_system() override = default;

    std::string name() const override { return "Event Driver System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, float dt) override;
    void shutdown(driver_kernel* kernel) override;

    scope<event_system>& events();
    const scope<event_system>& events() const;

    void trigger_event(const std::string_view name, const value& data);

    /// SDL event polling and input system dispatch
    void pump_events(driver_kernel& kernel);

    void handle_open_ui_window_event(const value& data);
    void handle_close_ui_window_event(const value& data);

   private:
    scope<event_system> event_system_ptr = nullptr;

    // void handle_list_driver_default_event(const value& data);
    // void handle_list_driver_windows_event(const value& data);
    // void handle_list_driver_files_event(const value& data);
    // void handle_list_driver_scenes_event(const value& data);
    // void handle_list_driver_assets_event(const value& data);

    // void handle_object_driver_create_event(const value& data);
    // void handle_object_driver_destroy_event(const value& data);
    // void handle_object_driver_push_event(const value& data);
    // void handle_object_driver_pop_event(const value& data);
    // void handle_object_driver_info_event(const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP