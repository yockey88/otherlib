/**
 * \file editor_driver.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_DRIVER_HPP
#define OTHER_EDITOR_EDITOR_DRIVER_HPP

#include "driver/driver.hpp"
#include "data-structures/std_container.hpp"

#include "edit_stack.hpp"
#include "editor_context.hpp"

namespace other {

  class OTHER_CLASS editor_driver : public driver {
   public:
    editor_driver(const command_line& cmd, const config_table& config)
        : driver(cmd, config), context(this) {}
    ~editor_driver() override {}

    void on_early_initialize() override;
    void on_initialize() override;
    void on_build_driver_input_map(input_map& map) override;
    void on_rendering_pipeline_loaded(natural_t asset_id, render_pipeline* pipeline) override;
    void on_rendering_pipeline_unloaded(natural_t asset_id, render_pipeline* pipeline) override;

    void update_running() override;
    void on_scene_activated(natural_t scene_id) override;
    void on_scene_played(natural_t scene_id) override;
    void on_scene_paused(natural_t scene_id) override {}
    void on_scene_stopped(natural_t scene_id) override;
    void on_scene_deactivated(natural_t scene_id) override;
    void on_begin_frame(render_data* data) override;

    void on_shutdown() override;

    struct mouse_state {
      glm::vec2 position = { 0, 0 };
      glm::vec2 delta = { 0, 0 };
    };

   private:
    editor_context context;
    natural_t viewport_id = 0;

    mouse_state mouse;
    bool pressing_mouse_wheel = false;
    bool move_toggled_on = false;

    void update_input();
    void save_active_scene();

    void on_create_project() override;

    void on_input_event(const input_state_change_event& event) override;

    ostd::vector<selected_draw> get_selection_draws() const;
    pipeline_definition get_debug_overlay_pipeline_definition() const;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_DRIVER_HPP