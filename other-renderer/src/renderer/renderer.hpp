/**
 * @file renderer/renderer.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_HPP

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/config_table.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/renderer_backend.hpp"

#include "pipeline_definition.hpp"

namespace other {

  class renderer_backend;

  class render_pipeline;
  class camera;

  struct debug_line {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec4 color;
  };
  struct debug_triangle {
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec4 color;
  };

  struct debug_rendering_data {
    std::vector<debug_line> debug_lines;
    std::vector<debug_triangle> debug_triangles;
  };

  struct render_data {
    glm::vec4 clear_color = glm::vec4(0.2f, 0.22f, 0.233f, 1.0f);

    camera* primary_camera = nullptr;
    const gpu::directional_light* scene_ambient_light = nullptr;
    std::vector<gpu::directional_light> ambient_lights;
    std::vector<gpu::point_light> point_lights;

    size_t num_draw_calls = 0;
    std::map<mesh_key, size_t> mesh_indices;
    std::vector<mesh_key> mesh_keys;
    std::vector<draw_call> draw_calls;
    std::vector<gpu::graphics_material_buffer> material_buffers;
    std::vector<gpu::model_matrix_buffer> model_buffers;
    std::vector<gpu::bone_matrix_buffer> bone_buffers;

    debug_rendering_data debug_data;
  };

  class renderer {
   public:
    renderer(const config_table& config)
        : config(config) {}
    virtual ~renderer() = default;

    void begin_frame(render_data* data);
    void render();
    void end_frame();

    opt<resource_handle> get_pipeline_output(const std::string_view pipeline_name) const;

    void begin_ui_frame();
    void end_ui_frame();

    inline const config_table& get_config() const { return config; }

    inline decltype(auto) get_pipeline_list() {
      return pipelines |
        std::views::values |
        std::views::filter([](render_pipeline* pipeline) { return pipeline != nullptr; }) |
        std::ranges::to<std::vector>();
    }

    glm::ivec2 get_window_size();
    void set_clear_color(const glm::vec4& color);

    glm::vec2 get_mouse_position();

    resource_handle create_resource(const std::string& name, resource_type type);
    void destroy_resource(const resource_handle& handle);

    bool resource_exists(const resource_handle& handle);

    template <typename T>
    T& get_resource(const resource_handle& handle) {
      return *rendering()->api()->get_resource_as<T>(handle);
    }

    void* get_texture_gpu_resource(const resource_handle& handle) {
      return rendering()->api()->get_texture_gpu_resource(handle);
    }

    void add_pipeline(const std::string_view name, pipeline_definition&& definition) {
      uint64_t hash = FNV(name);
      auto itr = pipelines.find(hash);
      if (itr != pipelines.end()) {
        CORE_LOG_ERROR("Pipeline with name [{}] already exists.", name);
        return;
      }

      auto* pl = arena_allocator<render_pipeline>{}.allocate(std::move(definition));
      pl->initialize_pipeline(this);
      auto [pitr, res] = pipelines.insert({ hash, pl });
      if (!res) {
        CORE_LOG_ERROR("Failed to insert pipeline [{}] into pipeline map.", name);
        pl->shutdown_pipeline();
        arena_allocator<render_pipeline>{}.free(pl);
        return;
      }
    }

    void remove_pipeline(const std::string_view name);

    virtual void execute_draw_calls(render_graph::node* current_node);

    constexpr static inline size_t kMaxDrawCalls = 1024;

   protected:
    renderer_backend* rendering();

   private:
    friend class render_graph;

    config_table config;

    frame_resources current_frame_resources;
    render_data* scene_data = nullptr;

    std::map<natural_t, render_pipeline*> pipelines;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_HPP