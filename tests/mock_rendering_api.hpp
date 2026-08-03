/**
 * \file mock_rendering_api.hpp
 */
#ifndef OTHER_TESTS_MOCK_RENDERING_API_HPP
#define OTHER_TESTS_MOCK_RENDERING_API_HPP

#include <gmock/gmock.h>

#include "renderer/rendering_api.hpp"

namespace other {

  class mock_rendering_api : public rendering_api {
   public:
    mock_rendering_api() = default;
    ~mock_rendering_api() override = default;

    MOCK_METHOD(void, on_initialize, (scope<window_manager> & window_mgr), (override));
    MOCK_METHOD(void, on_shutdown, (scope<window_manager> & window_mgr), (override));

    MOCK_METHOD(void, initialize_ui_context, (), (override));
    MOCK_METHOD(void, shutdown_ui_context, (), (override));

    MOCK_METHOD(void, handle_event, (SDL_Event * event), (override));

    MOCK_METHOD(void, set_clear_color, (const glm::vec4& color), (override));
    MOCK_METHOD(void, set_clear_depth, (float depth), (override));
    MOCK_METHOD(void, set_clear_stencil, (uint32_t stencil), (override));

    MOCK_METHOD(void, debug_group_begin, (const std::string_view name), (override));
    MOCK_METHOD(void, debug_group_end, (), (override));

    MOCK_METHOD(void, on_begin_frame, (scope<window_manager> & window_mgr), (override));
    MOCK_METHOD(void, on_end_frame, (scope<window_manager> & window_mgr), (override));

    MOCK_METHOD(void, begin_pass, (const pass_begin_info& info), (override));
    MOCK_METHOD(void, end_pass, (), (override));

    MOCK_METHOD(void, bind_set, (uint32_t set_index, std::span<const binding_record>), (override));
    MOCK_METHOD(void, set_dynamic_offsets, (uint32_t set_index, std::span<const uint32_t>), (override));

    MOCK_METHOD(void, execute_draw_call, (render_polygon_mode render_state, mesh::primitive_type draw_mode, const draw_call& call), (override));

    MOCK_METHOD(void, set_viewport, (int32_t x, int32_t y, int32_t width, int32_t height), (override));
    MOCK_METHOD(void, clear_viewport, (const glm::vec4& clear_color, uint32_t clear_mask), (override));
    MOCK_METHOD(void, set_color_mask, (bool enabled_or_disabled), (override));
    MOCK_METHOD(void, set_depth_mask, (bool enabled_or_disabled), (override));
    MOCK_METHOD(void, set_depth_test, (bool enabled_or_disabled), (override));
    MOCK_METHOD(void, set_blending, (bool enabled_or_disabled), (override));

    MOCK_METHOD(void, set_polygon_mode, (render_polygon_mode mode), (override));
    MOCK_METHOD(void, set_stencil_func, (stencil_func func, int32_t ref, uint32_t mask), (override));
    MOCK_METHOD(void, set_stencil_test, (bool enabled), (override));
    MOCK_METHOD(void, set_stencil_op, (stencil_op sfail, stencil_op dpfail, stencil_op dppass), (override));
    MOCK_METHOD(void, set_stencil_mask, (uint32_t mask), (override));
    MOCK_METHOD(void, set_depth_func, (depth_func func), (override));

    MOCK_METHOD(void, memory_barrier, (shader::compute_barrier_type bits), (override));

    MOCK_METHOD(void, begin_ui_frame_backend_newframe, (), (override));
    MOCK_METHOD(void, end_ui_frame_backend_draw_data, (), (override));

    MOCK_METHOD(void, bind_shader_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, unbind_shader_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, compile_and_attach_source, (const resource_handle& handle, const std::string_view source, shader::source_type type), (override));
    MOCK_METHOD(void, finalize_shader, (const resource_handle& handle), (override));
    MOCK_METHOD(void, dispatch_shader, (const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type), (override));

    MOCK_METHOD(void, bind_texture_resource, (const resource_handle& handle, uint32_t index), (override));
    MOCK_METHOD(void, unbind_texture_resource, (const resource_handle& handle, uint32_t index), (override));
    MOCK_METHOD(void, set_texture_filter, (const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter), (override));
    MOCK_METHOD(void, set_texture_wrap_mode, (const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t, texture::wrap wrap_r), (override));
    MOCK_METHOD(void, upload_texture, (const resource_handle& handle, texture::tex_type type, texture::format format, uint32_t mip_levels, bool generate_mipmaps, const glm::ivec2& img_size, uint32_t depth, void* data, size_t data_size), (override));
    MOCK_METHOD(void, bind_image, (const resource_handle& handle, uint32_t index, uint32_t level, bool layered, int32_t layer, texture::format frmt, access_flags flags), (override));
    MOCK_METHOD(void*, get_texture_gpu_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, blit_texture, (const blit_data& src, const blit_data& dest, const glm::ivec3& size), (override));

    MOCK_METHOD(void, bind_buffer_resource, (const resource_handle& handle, gpu_buffer::buf_type type), (override));
    MOCK_METHOD(void, unbind_buffer_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, bind_shader_buffer_resource, (const resource_handle& handle, const resource_handle& shader_handle, const std::string_view name, uint32_t binding_point, gpu_buffer::buf_type buffer_type, const void* data, size_t size), (override));
    MOCK_METHOD(void, set_shader_block_binding, (const resource_handle& shader_handle, const std::string_view name, uint32_t binding_point, gpu_buffer::buf_type buffer_type), (override));
    MOCK_METHOD(void, buffer_data, (const resource_handle& handle, uint32_t binding_point, const void* data, size_t size), (override));
    MOCK_METHOD(void, buffer_range, (const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data), (override));

    MOCK_METHOD(void, bind_mesh_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, unbind_mesh_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, set_mesh_vertex_attributes, (const resource_handle& handle, const std::span<const vertex_attribute> attributes), (override));
    MOCK_METHOD(void, draw_mesh, (const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count, mesh::attribute_type index_type), (override));
    MOCK_METHOD(void, draw_mesh_instanced, (const resource_handle& handle, const draw_call& call), (override));

    MOCK_METHOD(void, bind_framebuffer_resource, (const resource_handle& handle, bool clear), (override));
    MOCK_METHOD(void, unbind_framebuffer_resource, (const resource_handle& handle), (override));
    MOCK_METHOD(void, framebuffer_texture_2d, (const resource_handle& handle, const resource_handle& texture, framebuffer::attachment_type type, uint32_t mip_level, uint32_t color_attachment_index), (override));
    MOCK_METHOD(void, finalize_framebuffer, (const resource_handle& handle), (override));

    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, int8_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, uint8_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, int16_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, uint16_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, int32_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, uint32_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, int64_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, uint64_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, real_t value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, const glm::vec2& value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, const glm::vec3& value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, const glm::vec4& value), (override));
    MOCK_METHOD(void, set_shader_uniform, (const resource_handle& shader, const std::string_view name, const glm::mat4& value, bool transpose), (override));

    MOCK_METHOD(uint32_t, uniform_buffer_offset_alignment, (), (const, override));
    MOCK_METHOD(uint32_t, storage_buffer_offset_alignment, (), (const, override));

    MOCK_METHOD(int32_t, get_gpu_api_window_flags, (), (const, override));

    MOCK_METHOD(framebuffer*, create_framebuffer_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_framebuffer_resource, (const resource_handle& handle), (override));

    MOCK_METHOD(mesh*, create_mesh_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_mesh_resource, (const resource_handle& handle), (override));

    MOCK_METHOD(gpu_buffer*, create_buffer_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_buffer_resource, (const resource_handle& handle), (override));

    MOCK_METHOD(texture*, create_texture_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_texture_resource, (const resource_handle& handle), (override));

    MOCK_METHOD(cube_map*, create_cube_map_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_cube_map_resource, (const resource_handle& handle), (override));

    MOCK_METHOD(shader*, create_shader_resource, (const resource_handle& handle, resource_type type), (override));
    MOCK_METHOD(void, destroy_shader_resource, (const resource_handle& handle), (override));
  };

}  // namespace other

#endif  // OTHER_TESTS_MOCK_RENDERING_API_HPP
