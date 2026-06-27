/**
 * \file ui/viewport/viewport.cpp
 **/
#include "ui/viewport/viewport.hpp"

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "renderer/render_pipeline.hpp"

#include "driver/driver.hpp"
#include "ui/ui_node.hpp"
#include "ui/viewport/viewport_node.hpp"

namespace other {
  namespace ui {

    viewport::viewport(editor_context& ctx, event_system& events, renderer& renderer_instance, driver* driver_ptr)
        : ui_window(&events, "Viewport", true, ImGuiWindowFlags_MenuBar), driver_ptr(driver_ptr), editor_ctx(ctx) {
      events.register_event("viewport.resize");
      add_node(make_ref<viewport_node>(renderer_instance, driver_ptr, this, "ViewportNode"));

      // renderer_instance.register_debug_pass("editor-grid", {
      //                                                        .name = "editor-grid",
      //                                                        .element_size = 2 * (sizeof(float) * 3) + (sizeof(float) * 4),  /// sizeof(debug_line)
      //                                                        .max_per_frame = 100,
      //                                                        .draw_recipe = {
      //                                                          .topology = mesh::primitive_type::LINES,
      //                                                          .vertex_layout = {
      //                                                            { value_type::VEC3, "position", 0, 0 },
      //                                                            { value_type::VEC3, "color", 1, sizeof(float) * 3 },
      //                                                          },
      //                                                        },
      //                                                      });
      auto& debug_reg = renderer_instance.get_debug_stream_registry();
      debug_reg.register_stream("editor-grid", {
                                                 .name = "editor-grid",
                                                 .element_size = 2 * (sizeof(float) * 3) + (sizeof(float) * 4),  /// sizeof(debug_line)
                                                 .max_per_frame = 100,
                                                 .draw_recipe = {
                                                   .topology = mesh::primitive_type::LINES,
                                                   .vertex_layout = {
                                                     { value_type::VEC3, "position", 0, 0 },
                                                     { value_type::VEC3, "color", 1, sizeof(float) * 3 },
                                                   },
                                                 },
                                               });
    }

    void viewport::on_render_header() {
      ui::menu options_menu = {
        "Options"
      };
      ui::menu view_menu = {
        "View"
      };
      ui::menu tools_menu = {
        "Tools"
      };
      ui::menu help_menu = {
        "Help"
      };
    }

    void viewport::on_render_body() {
    }

    void viewport::on_pre_render_nodes() {
    }

  }  // namespace ui
}  // namespace other