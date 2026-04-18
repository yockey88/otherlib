/**
 * \file renderer/render_pipeline.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_PIPELINE_HPP
#define OTHER_RENDERER_RENDER_PIPELINE_HPP

#include <algorithm>
#include <string_view>

#include <imgui/imgui.h>

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "renderer/pipeline_definition.hpp"
#include "renderer/render_graph.hpp"

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
    render_pipeline() = default;
    render_pipeline(pipeline_definition&& def)
        : definition(std::move(def)) {}
    render_pipeline(const pipeline_definition& def)
        : definition(def) {}
    virtual ~render_pipeline() = default;

    void initialize_pipeline(renderer* renderer_ptr);
    void shutdown_pipeline();

    bool reload(pipeline_definition&& new_def);

    void prepare_frame(render_data* data);
    void render_frame(renderer* renderer_ptr);

    ImTextureID get_final_output_texture_id();
    resource_handle get_screen_texture() const;
    frame_resources get_frame_resources() const;

    const pipeline_definition& get_definition() const { return definition; }
    const std::string& get_name() const { return definition.name; }
    inline bool is_valid() const { return valid; }

   private:
    pipeline_definition definition;
    bool valid = false;

    render_graph* graph = nullptr;
    render_data* frame_render_data = nullptr;

    renderer* renderer_ptr = nullptr;

    struct named_resource {
      std::string name;
      resource_handle handle;
      resource_tag tag = resource_tag::NONE;
    };

    std::map<natural_t, named_resource> buffer_resources;   /// keyed by FNV(name)
    std::map<natural_t, named_resource> texture_resources;  /// keyed by FNV(name)
    std::map<std::string, resource_handle> shader_handles;  /// keyed by shader def name

    std::map<resource_tag, resource_handle> tagged_buffer_handles;
    std::map<resource_tag, resource_handle> tagged_texture_handles;

    opt<resource_handle> screen_texture_handle;
    opt<resource_handle> quad_mesh_handle;

    using executor_fn = render_graph::pass_executor;
    std::map<std::string, executor_fn> executor_overrides;

    void upload_buffer(resource_handle handle, const void* data, size_t size);
    opt<resource_handle> find_buffer_by_name(const std::string_view name) const;
    opt<resource_handle> find_texture_by_name(const std::string_view name) const;
    opt<resource_handle> find_tagged(resource_tag tag) const;
    shader* get_pass_shader(const std::string_view pass_name);

    void override_pass_executor(const std::string_view pass_name, executor_fn&& fn);

    void create_resources_from_def();

    void build_tag_maps();
    void build_passes_from_def();
    void validate();
    void destroy_resources();

    void build_pass(const pipeline_pass_definition& pass_def, render_graph::pass_builder& builder);
    void upload_to_handle(resource_handle handle, const void* data, size_t size);

    renderer* get_renderer() const;
    glm::ivec2 resolve_size(bool use_window, const glm::ivec2& fixed) const;
    bool is_buffer_resource(const std::string_view name) const;

    executor_fn make_executor(const pipeline_pass_definition& pass);
    executor_fn make_draw_scene_executor();
    executor_fn make_fullscreen_quad_executor(const pipeline_executor_definition& exec, const std::string& pass_name);
    executor_fn make_noop_executor();

    opt<resource_handle> get_shader_handle(const std::string_view shader_name) const;

    static void apply_uniforms(shader& s, const std::map<std::string, value>& uniforms);
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDER_PIPELINE_HPP