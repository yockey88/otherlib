/**
 * \file renderer/render_pipeline.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_PIPELINE_HPP
#define OTHER_RENDERER_RENDER_PIPELINE_HPP

#include <string_view>

#include <imgui/imgui.h>

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/frame_node.hpp"
#include "renderer/pass_runtime.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/resource_tag.hpp"

namespace other {

  struct render_data;
  class renderer;

  struct frame_resources {
    std::map<resource_tag, resource_handle> tagged_buffers;
    std::map<resource_tag, resource_handle> tagged_textures;

    inline opt<resource_handle> find(resource_tag tag) const {
      if (auto itr = tagged_buffers.find(tag); itr != tagged_buffers.end()) {
        return itr->second;
      }
      if (auto itr = tagged_textures.find(tag); itr != tagged_textures.end()) {
        return itr->second;
      }
      return std::nullopt;
    }

    inline bool has(resource_tag tag) const {
      return tagged_buffers.contains(tag) || tagged_textures.contains(tag);
    }
  };

  class render_pipeline {
   public:
    static constexpr uint32_t kMaxFramesInFlight = 3;

    render_pipeline() = default;
    render_pipeline(pipeline_definition&& def)
        : definition(std::move(def)) {}
    render_pipeline(const pipeline_definition& def)
        : definition(def) {}
    virtual ~render_pipeline() = default;

    void initialize_pipeline(renderer* renderer_ptr);
    void shutdown_pipeline();

    bool has_pass(natural_t pass_id) const;
    pass_runtime& get_pass_runtime(natural_t pass_id);

    bool reload(pipeline_definition&& new_def);
    void rebuild();

    void prepare_frame(render_data* data);
    void bind_frame_resources(const render_data& data);
    void bind_draw_resources(pass_runtime& runtime, const render_data& data, size_t draw_index);
    void render_frame(renderer* renderer_ptr);
    void reset_draw_buffers();
    glm::ivec2 get_window_size() const;

    ImTextureID get_final_output_texture_id();
    ImTextureID get_texture_id(const std::string_view name);
    resource_handle get_screen_texture() const;
    frame_resources get_frame_resources() const;

    const pipeline_definition& get_definition() const { return definition; }
    const std::string& get_name() const { return definition.name; }
    inline bool is_valid() const { return valid; }

    void upload_buffer(resource_handle handle, const void* data, size_t size);
    void upload_to_handle(resource_handle handle, const void* data, size_t size);
    opt<resource_handle> find_buffer_by_name(const std::string_view name) const;
    opt<resource_handle> find_texture_by_name(const std::string_view name) const;
    opt<resource_handle> find_tagged(resource_tag tag) const;
    shader* get_pass_shader(const std::string_view pass_name);

    resource_handle get_quad_mesh_handle() const;

    static void apply_uniforms(shader& s, const std::map<std::string, value>& uniforms);

   private:
    struct named_resource {
      std::string name;
      resource_handle handle;
      resource_tag tag = resource_tag::none();
    };
    struct debug_pass {
      std::string name;
      render_pass* pass;
      pass_runtime* runtime;
      render_graph::pass_executor executor;

      std::map<natural_t, render_pass::buffer_resource> input_buffers;
      std::map<natural_t, render_pass::buffer_resource> output_buffers;
      std::map<natural_t, render_pass::texture_resource> input_textures;
      std::map<natural_t, render_pass::texture_resource> output_textures;
    };

    pipeline_definition definition;
    bool valid = false;

    render_graph* graph = nullptr;
    render_data* frame_render_data = nullptr;

    renderer* renderer_ptr = nullptr;

    std::map<natural_t, named_resource> buffer_resources;   /// keyed by FNV(name)
    std::map<natural_t, named_resource> texture_resources;  /// keyed by FNV(name)
    std::map<natural_t, resource_handle> shader_handles;    /// keyed by shader def name

    std::map<resource_tag, resource_handle> tagged_buffer_handles;
    std::map<resource_tag, resource_handle> tagged_texture_handles;

    opt<resource_handle> screen_texture_handle;
    opt<resource_handle> quad_mesh_handle;

    std::map<natural_t, pass_runtime> pass_runtimes;

    using executor_fn = render_graph::pass_executor;
    std::map<std::string, executor_fn> executor_overrides;

    std::vector<debug_pass> debug_passes;

    // for avoiding resource collisions in rendering backend when user loads multiple pipelines with same resource names
    std::string get_pipeline_name(const std::string_view n) const;

    void build_pass_runtimes();
    void destroy_pass_runtimes();

    void override_pass_executor(const std::string_view pass_name, executor_fn&& fn);

    void create_resources_from_def();

    void build_tag_maps();
    void build_passes_from_def();
    void validate();
    void destroy_resources();

    void build_pass(const pipeline_pass_definition& pass_def, render_graph::pass_builder& builder);

    renderer* get_renderer() const;
    glm::ivec2 resolve_size(bool use_window, const glm::ivec2& fixed) const;
    bool is_buffer_resource(const std::string_view name) const;

    executor_fn make_executor(const pipeline_pass_definition& pass);

    opt<resource_handle> get_shader_handle(const std::string_view shader_name) const;
    pass_runtime& build_pass_runtime(pass_runtime& runtime, render_pass* pass, const pipeline_pass_definition& pass_def);
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDER_PIPELINE_HPP