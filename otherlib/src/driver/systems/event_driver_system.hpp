/**
 * \file driver/systems/event_driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP

#include "core/value.hpp"

#include "driver/systems/driver_system.hpp"

namespace other {

  class event_driver_system : public core_system<event_driver_system> {
   public:
    event_driver_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::EVENT_DRIVER_SYSTEM) {}
    ~event_driver_system() override = default;

    std::string name() const override { return "Event Driver System"; }

    void initialize() override;

    void handle_scene_load_empty_event(const value& data);
    void handle_scene_load_event(const value& data);
    void handle_scene_unload_event(const value& data);
    void handle_scene_info_event(const value& data);
    void handle_scene_playback_command_event(const value& data);

    void handle_open_ui_window_event(const value& data);
    void handle_close_ui_window_event(const value& data);

    void handle_list_driver_default_event(const value& data);
    void handle_list_driver_windows_event(const value& data);
    void handle_list_driver_files_event(const value& data);
    void handle_list_driver_scenes_event(const value& data);
    void handle_list_driver_assets_event(const value& data);

    void handle_object_driver_create_event(const value& data);
    void handle_object_driver_destroy_event(const value& data);
    void handle_object_driver_push_event(const value& data);
    void handle_object_driver_pop_event(const value& data);
    void handle_object_driver_info_event(const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_EVENT_DRIVER_SYSTEM_HPP