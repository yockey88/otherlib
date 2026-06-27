/**
 * \file ui/viewport/viewport.cpp
 **/
#include "ui/viewport/viewport.hpp"

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "renderer/render_pipeline.hpp"

#include "object/render_component.hpp"

#include "driver/driver.hpp"
#include "ui/ui_node.hpp"
#include "ui/viewport/viewport_node.hpp"

namespace other {
  namespace ui {
    namespace detail {

      render_graph::pass_executor make_stencil(editor_context* ctx, const pipeline_pass_definition& def, render_pipeline* pl) {
        return [editor_ctx = ctx, def, pl](pass_context& ctx) {
          OTHER_ASSERT(editor_ctx != nullptr, "Pass executor context is null in draw_selected_geometry executor for pass '{}'", def.name);
          auto* sh = pl->get_pass_shader(def.name);
          OTHER_ASSERT(sh != nullptr, "No shader bound for pass '{}'", def.name);
          if (editor_ctx->current_selection.objects.empty()) {
            return;
          }
          auto& api = ctx.get_renderer().rendering()->api();

          api->set_depth_test(true);
          api->set_stencil_func(STENCIL_ALWAYS, 1, 0xFF);
          api->set_stencil_mask(0xFF);

          for (const auto& obj_id : editor_ctx->current_selection.objects) {
            render_component* rc = editor_ctx->active_scene->get_component<render_component>(obj_id);
            if (rc == nullptr) {
              continue;
            }

            ctx.get_renderer().get_resource<mesh>(rc->obj_model.source->get_mesh_handle()).draw();
          }
        };
      }

      render_graph::pass_executor make_stencil_outline(editor_context* ctx, const pipeline_pass_definition& def, render_pipeline* pl) {
        return [editor_ctx = ctx, def, pl](pass_context& ctx) {
          OTHER_ASSERT(editor_ctx != nullptr, "Pass executor context is null in draw_selected_geometry executor for pass '{}'", def.name);
          auto* sh = pl->get_pass_shader(def.name);
          OTHER_ASSERT(sh != nullptr, "No shader bound for pass '{}'", def.name);
          if (editor_ctx->current_selection.objects.empty()) {
            return;
          }
          auto& api = ctx.get_renderer().rendering()->api();

          api->set_depth_test(true);
          api->set_stencil_func(STENCIL_NOTEQUAL, 1, 0xFF);
          api->set_stencil_mask(0x00);

          for (const auto& obj_id : editor_ctx->current_selection.objects) {
            render_component* rc = editor_ctx->active_scene->get_component<render_component>(obj_id);
            if (rc == nullptr) {
              continue;
            }

            ctx.get_renderer().get_resource<mesh>(rc->obj_model.source->get_mesh_handle()).draw();
          }
        };
      }

    }  // namespace detail

    viewport::viewport(editor_context& ctx, event_system& events, renderer& renderer_instance, driver* driver_ptr)
        : ui_window(&events, "Viewport", true, ImGuiWindowFlags_MenuBar), renderer_instance(renderer_instance), driver_ptr(driver_ptr), editor_ctx(ctx) {
      events.register_event("viewport.resize");
      add_node(make_ref<viewport_node>(renderer_instance, driver_ptr, this, "ViewportNode"));
    }

    void viewport::initialize_debug_passes() {
      auto win_size = renderer_instance.get_window_size();
      resource_handle stencil_tex = texture::create("viewport-stencil", texture::TEXTURE_2D, texture::format::RGBA8, win_size.x, win_size.y);
      display_texture_id = texture::create("viewport-display", texture::TEXTURE_2D, texture::format::RGBA32F, win_size.x, win_size.y);

      renderer_instance.register_texture_resource("default-instancing", "viewport-stencil", stencil_tex);
      renderer_instance.register_texture_resource("default-instancing", "viewport-display", display_texture_id);
      renderer_instance.register_shader_resource("stencil-shader", "resources/basic-textured-quad.vert", "resources/stencil-shader.frag");
      renderer_instance.register_shader_resource("select-outline-shader", "resources/basic-textured-quad.vert", "resources/basic-solid-color.frag");

      auto& reg = renderer_instance.get_executor_registry();
      reg.register_executor("stencil", std::bind_front(detail::make_stencil, &editor_ctx));
      reg.register_executor("stencil_outline", std::bind_front(detail::make_stencil_outline, &editor_ctx));

      pipeline_pass_definition stencil_pass_def = {
        .name = "viewport-stencil-pass",
        .pass_type = render_pass::RENDER_PASS,
        .shader_name = "stencil-shader",

        .use_window_size = true,
        .create_framebuffer = true,
        .clear_color = glm::vec4(0.f),

        .inputs = {
          {
            .resource_name = "smaa-texture",
            .binding = 0,
            .access = access_flags::READ,
          },
        },
        .outputs = {
          {
            .resource_name = "viewport-stencil",
            .access = access_flags::WRITE,
          },
        },
        .executor = {
          .name = "stencil",
        },
        .bindings = {
          {
            .name = "per_draw.model_buffer",
            .scope = binding_scope::PER_DRAW_CALL,
            .type = binding_type::UNIFORM_BUFFER,
            .binding = 0,
            .element_size = sizeof(gpu::model_matrix_buffer),
          },
        }
      };

      pipeline_pass_definition select_pass_def = {
        .name = "viewport-outline-pass",
        .pass_type = render_pass::RENDER_PASS,

        .shader_name = "stencil_pass",

        .use_window_size = true,
        .create_framebuffer = true,
        .clear_color = glm::vec4(0.f),

        .inputs = {
          {
            .resource_name = "viewport-stencil",
            .binding = 0,
            .access = access_flags::READ,
          },
        },
        .outputs = {
          {
            .resource_name = "viewport-display",
            .binding = 1,
            .access = access_flags::READ,
          },
        },
        .executor = {
          .name = "stencil_outline",
        },
        .bindings = {
          {
            .name = "per_draw.model_buffer",
            .scope = binding_scope::PER_DRAW_CALL,
            .type = binding_type::UNIFORM_BUFFER,
            .binding = 0,
            .element_size = sizeof(gpu::model_matrix_buffer),
          },
        }
      };

      // renderer_instance.register_debug_pass("default-instancing", stencil_pass_def.name, stencil_pass_def);
      // renderer_instance.register_debug_pass("default-instancing", select_pass_def.name, select_pass_def);
      // renderer_instance.rebuild_pipeline("default-instancing");
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