/**
 * @file renderer/renderer.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_RENDERER_HPP
#define OTHER_RENDERER_RENDERER_RENDERER_HPP

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/config_table.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/camera.hpp"
#include "renderer/debug_render_stream.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/frame_binding_registry.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/pass_executor_resolver.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_executor_registry.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  class renderer_backend;

  struct frame_node;
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

  struct render_data {
    glm::vec4 clear_color = glm::vec4(0.2f, 0.22f, 0.233f, 1.0f);

    camera* primary_camera = nullptr;             //< tag = 'main-camera'
    gpu::light* sun_directional_light = nullptr;  //< tag = 'sun'
    std::vector<gpu::light> lights;
    gpu::simulation_environment_buffer simulation_environment;

    size_t num_draw_calls = 0;
    std::map<mesh_key, size_t> mesh_indices;
    std::vector<mesh_key> mesh_keys;
    std::vector<draw_call> draw_calls;
    std::vector<gpu::graphics_material_buffer> material_buffers;
    std::vector<gpu::model_matrix_buffer> model_buffers;
    std::vector<gpu::bone_matrix_buffer> bone_buffers;

    debug_streams debug_data;
  };

  class renderer {
   public:
    renderer(const config_table& config)
        : config(config) {}
    virtual ~renderer() = default;

    render_executor_registry& get_executor_registry() { return executor_registry; }
    frame_binding_registry& get_binding_registry() { return binding_registry; }
    pass_executor_resolver* get_pass_executor_resolver() { return pass_exec_resolver; }
    debug_stream_registry& get_debug_stream_registry() { return debug_stream_registry; }

    void initialize_pass_resolver(pass_executor_resolver* resolver);

    void register_debug_pass(const std::string_view pipeline_name, const std::string_view name, const pipeline_pass_definition& definition);

    void register_texture_resource(const std::string_view pipeline, const std::string_view name, resource_handle handle);
    void register_buffer_resource(const std::string_view pipeline, const std::string_view name, resource_handle handle);

    void register_shader_resource(const std::string_view pipeline, const std::string_view name, const filepath& vert_path, const filepath& geom_path, const filepath& frag_path);
    void register_shader_resource(const std::string_view pipeline, const std::string_view name, const filepath& comp_path);
    void register_shader_resource(const std::string_view pipeline, const std::string_view name, resource_handle handle);

    void rebuild_pipeline(const std::string_view pipeline_name);

    void begin_frame(render_data* data);
    void bind_frame_bindings(const render_data& data);
    void render();
    void end_frame();

    inline void set_override_camera(const camera& cam) { forced_camera = cam; }
    inline void set_should_force_camera(bool force) { should_force_camera = force; }
    inline bool override_camera() const { return should_force_camera; }
    inline camera* get_override_camera() {
      return should_force_camera ? &forced_camera : nullptr;
    }

    ImTextureID get_texture_id(const std::string_view pipeline, const std::string_view name);

    opt<resource_handle> find_texture_resource(const std::string_view name) const;
    opt<resource_handle> find_buffer_resource(const std::string_view name) const;

    opt<resource_handle> get_pipeline_output(const std::string_view pipeline_name) const;
    resource_handle get_or_create_debug_stream_mesh(std::string_view stream_name, const debug_stream_recipe& recipe);
    opt<resource_handle> get_debug_stream_shader_handle(std::string_view shader_name);

    void begin_ui_frame();
    void end_ui_frame();

    bool executor_resolves(const std::string_view name) const;
    bool frame_binder_resolves(const resource_tag& tag) const;
    bool draw_binder_resolves(const resource_tag& tag) const;
    bool instance_binder_resolves(const resource_tag& tag) const;
    render_graph::pass_executor attempt_executor_resolution(const std::string_view name, const pipeline_pass_definition& def, render_pipeline* pl);

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

    void add_pipeline(const std::string_view name, const pipeline_definition& definition) {
      uint64_t hash = FNV(name);
      auto itr = pipelines.find(hash);
      if (itr != pipelines.end()) {
        CORE_LOG_ERROR("Pipeline with name [{}] already exists.", name);
        return;
      }

      auto* pl = arena_allocator<render_pipeline>{}.allocate(definition);
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

    virtual void draw_mesh(const resource_handle& mesh_handle);
    virtual void execute_draw_calls(frame_node* current_node);

    constexpr static inline size_t kMaxDrawCalls = 1024;

    renderer_backend* rendering();

   private:
    friend struct frame_node;
    friend class render_pipeline;
    friend class render_graph;
    friend class pass_context;

    config_table config;

    bool should_force_camera = false;
    camera forced_camera;

    frame_resources current_frame_resources;
    render_data* scene_data = nullptr;

    pass_executor_resolver* pass_exec_resolver = nullptr;
    render_executor_registry executor_registry;
    frame_binding_registry binding_registry;
    debug_stream_registry debug_stream_registry;

    std::map<natural_t, resource_handle> debug_stream_meshes;
    std::map<natural_t, resource_handle> debug_stream_shaders;
    std::map<natural_t, render_pipeline*> pipelines;

    render_pipeline* get_pass_pipeline(natural_t pass_id) const;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDERER_HPP