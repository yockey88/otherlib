/**
 * \file editor_driver.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_DRIVER_HPP
#define OTHER_EDITOR_EDITOR_DRIVER_HPP

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS editor_driver : public driver {
   public:
    editor_driver(const config_table& config)
        : driver(config) {}
    ~editor_driver() override {}

    void on_initialize(const command_line&) override;
    void on_build_driver_input_map(input_map& map) override;
    void on_viewport_resize(const glm::vec2& size) override;

    void on_initialize_ui(scope<driver_ui>& ui_ptr) override;
    void on_shutdown() override {}

    void update_running() override;
    struct mouse_state {
      glm::vec2 position = { 0, 0 };
      glm::vec2 delta = { 0, 0 };
    };

   private:
    natural_t suzanne_obj_id = 0;

    natural_t suzanne_id = 0;
    bool suzanne_loaded = false;

    natural_t camera_obj_id = 0;

    mouse_state mouse;
    bool pressing_mouse_wheel = false;
    bool move_toggled_on = false;

    void on_input_event(const input_state_change_event& event) override;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_DRIVER_HPP