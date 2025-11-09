/**
 * \file renderer/render_pipeline.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_PIPELINE_HPP
#define OTHER_RENDERER_RENDER_PIPELINE_HPP

#include <algorithm>
#include <string_view>

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/gpu_buffer.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer.hpp"

namespace other {

  struct render_data;
  class renderer;

  class render_pipeline {
   public:
    render_pipeline() = default;
    virtual ~render_pipeline() = default;

    void initialize_pipeline(renderer* renderer);
    void shutdown_pipeline();

    void upload_buffer(const std::string_view name, const void* data, size_t size);

    void prepare_frame(renderer::frame_resources* resources, render_data* data);
    virtual void render_frame(renderer* renderer_ptr);

    renderer::frame_resources get_frame_resources() const;

    inline const bool is_valid() const { return valid; }

   protected:
    virtual void on_prepare_frame(renderer::frame_resources* resources, render_data* data) {}
    virtual void on_render_frame(renderer* renderer_ptr, render_data* data) {}
    virtual void create_resources() = 0;
    virtual void build_render_passes() = 0;
    void destroy_resources();

    renderer* get_renderer() {
      OTHER_ASSERT(graph != nullptr, "Render graph is not initialized. Cannot retrieve renderer.");
      return graph->get_renderer();
    }
    render_data* get_frame_render_data() {
      OTHER_ASSERT(frame_render_data != nullptr, "Frame render data is not set. Cannot retrieve it.");
      return frame_render_data;
    }

    void set_material_buffer(const std::string_view);
    void set_model_buffer(const std::string_view);
    void set_bone_buffer(const std::string_view);
    void set_point_light_buffer(const std::string_view name);
    void set_direction_light_buffer(const std::string_view name);
    void set_camera_buffer(const std::string_view name);

    void add_buffer_resource(const std::string_view name, gpu_buffer::buf_type type, gpu_buffer::usage usage);
    void add_texture_resource(const std::string_view name, const glm::vec2& size, texture::tex_type tex_type, texture::format format);
    void add_texture_resource(const std::string_view name, const glm::vec2& size, texture::tex_type tex_type, texture::format format, const std::pair<texture::filter, texture::filter>& filters, const std::tuple<texture::wrap, texture::wrap, texture::wrap>& wraps);

    template <typename T>
    T* get_resource(const std::string_view name) {
      auto itr = std::ranges::find_if(buffer_resources, [&](const auto& pair) { return pair.second.name == name; });
      if (itr == buffer_resources.end()) {
        CORE_LOG_ERROR("Resource [{}] not found in pipeline.", name);
        return nullptr;
      }
      return &get_renderer()->get_resource<T>(itr->second.handle);
    }

    shader* get_pass_shader(const std::string_view name);

    struct pass_builder {
      pass_builder(render_pipeline* pipeline, render_graph::pass_builder&& builder)
          : pipeline(pipeline), builder(std::move(builder)) {}

      pass_builder& clear_color(const glm::vec4& clear_color);
      pass_builder& buffer_resource(const std::string_view name, uint32_t binding, access_flags flags);
      pass_builder& texture_resource(const std::string_view name, framebuffer::attachment_type type, access_flags flags = READ_WRITE);
      pass_builder& execution_callback(render_graph::pass_executor&& executor, void* user_data = nullptr);
      // pass_builder& set_user_data(void* user_data);
      void end_pass();

      std::string pass_name;

     private:
      uint32_t curr_texture_slot = 0;

      render_pipeline* pipeline = nullptr;
      render_graph::pass_builder builder;
    };
    render_pipeline::pass_builder start_pass(const std::string_view name, opt<resource_handle> shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer = true);

    opt<resource_handle> find_buffer_resource(const std::string_view name) const;
    opt<resource_handle> find_texture_resource(const std::string_view name) const;

   private:
    render_graph* graph = nullptr;
    bool valid = false;

    render_data* frame_render_data = nullptr;
    renderer::frame_resources frame_resources;

    opt<resource_handle> material_buffer_handle;
    opt<resource_handle> model_buffer_handle;
    opt<resource_handle> bone_buffer_handle;
    opt<resource_handle> point_light_buffer_handle;
    opt<resource_handle> direction_light_buffer_handle;
    opt<resource_handle> camera_buffer_handle;

    struct resource {
      std::string name;
      resource_handle handle;
    };
    std::map<natural_t, resource> buffer_resources;
    std::map<natural_t, resource> texture_resources;

    void set_core_buffer(opt<resource_handle>& handle, const std::string_view name);
    void validate_pipeline();
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDER_PIPELINE_HPP