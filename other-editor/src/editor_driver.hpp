/**
 * \file editor_driver.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_DRIVER_HPP
#define OTHER_EDITOR_EDITOR_DRIVER_HPP

#include "driver/driver.hpp"

#include "edit_stack.hpp"
#include "editor_context.hpp"

namespace other {

  class OTHER_CLASS editor_driver : public driver {
   public:
    editor_driver(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    ~editor_driver() override {}

    void on_initialize() override;
    void on_build_driver_input_map(input_map& map) override;
    void on_viewport_resize(const glm::vec2& size) override;
    void on_shutdown() override {}

    void update_running() override;
    struct mouse_state {
      glm::vec2 position = { 0, 0 };
      glm::vec2 delta = { 0, 0 };
    };

   private:
    mouse_state mouse;
    bool pressing_mouse_wheel = false;
    bool move_toggled_on = false;

    input_map get_default_editor_input_map();
    void on_input_event(const input_state_change_event& event) override;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_DRIVER_HPP