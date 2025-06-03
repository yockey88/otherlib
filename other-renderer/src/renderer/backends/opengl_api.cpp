/**
 * \file renderer/backends/opengl_api.cpp
 **/
#include "renderer/backends/opengl_api.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "core/fnv.hpp"
#include "core/logger.hpp"

namespace other {
  namespace {

    SDL_GLContext gl_ctx(void* handle) {
      return static_cast<SDL_GLContext>(handle);
    }

    void check_for_gl_error(const char* func_name, const char* file, int line);

  }  // namespace

}  // namespace other

#define CHECKGL()                                     \
  do {                                                \
    check_for_gl_error(__func__, __FILE__, __LINE__); \
  } while (0)

namespace other {

  opengl_api::~opengl_api() {}

  void opengl_api::on_initialize(scope<window_manager>& window_mgr) {
    if (native_window() == nullptr) {
      CORE_LOG_ERROR("Native window handle is null.");
      return;
    }
    SDL_Window* window = native_window();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    // SDL_GL_SetSwapInterval(0);

    SDL_GLContext gpu_context = SDL_GL_CreateContext(window);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      CORE_LOG_ERROR("Failed to initialize GLAD: {}", SDL_GetError());
      return;
    }

    if (gpu_context == nullptr) {
      CORE_LOG_ERROR("Failed to create OpenGL context: {}", SDL_GetError());
      return;
    }
    CHECKGL();

    // PROFILE_GPU_CONTEXT;

    std::string gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::string gl_renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    CORE_LOG_DEBUG("OpenGL Version: {}", gl_version);
    CORE_LOG_DEBUG("OpenGL Renderer: {}", gl_renderer);
    set_gpu_context(gpu_context);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // glEnable(GL_STENCIL_TEST);
    // glStencilFunc(GL_ALWAYS, 1, 0xFF);
    // glStencilMask(0xFF);
    // glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    SDL_GL_MakeCurrent(window_mgr->get_main_window(), gl_ctx(get_gpu_context()));
    CORE_LOG_INFO("OpenGL API initialized successfully.");
  }

  void opengl_api::on_shutdown(scope<window_manager>& window_mgr) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot shutdown.");
      return;
    }

    gpu_resources.clear();
    resource_types.clear();
    in_process_resources.clear();
    shader_resources.clear();
    shader_uniforms.clear();
    texture_resources.clear();
    buffer_resources.clear();
    mesh_resources.clear();
    resource_handles.clear();

    SDL_GLContext ctx = gl_ctx(get_gpu_context());
    if (ctx == nullptr) {
      CORE_LOG_ERROR("OpenGL context is already null, cannot shutdown.");
      return;
    }

    SDL_GL_DestroyContext(ctx);
    set_gpu_context(nullptr);

    CORE_LOG_INFO("OpenGL API shutdown successfully.");
  }

  void opengl_api::initialize_ui_context() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot initialize UI context.");
      return;
    }

    ImGui_ImplSDL3_InitForOpenGL(native_window(), get_context_handle());
    ImGui_ImplOpenGL3_Init("#version 460 core");
  }

  void opengl_api::shutdown_ui_context() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot shutdown UI context.");
      return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
  }

  void opengl_api::handle_event(SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
      SDL_Window* window = native_window();
      int width, height;
      SDL_GetWindowSize(window, &width, &height);
      glViewport(0, 0, width, height);
    }

    ImGui_ImplSDL3_ProcessEvent(event);
  }

  void opengl_api::set_clear_color(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    override_clear_color(color);
  }

  void opengl_api::on_begin_frame(scope<window_manager>& window_mgr) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot begin frame.");
      return;
    }

    SDL_GLContext ctx = gl_ctx(get_gpu_context());
    if (ctx == nullptr) {
      CORE_LOG_ERROR("OpenGL context is null, cannot begin frame.");
      return;
    }

    // // glViewport(0, 0, get_window_size().x, get_window_size().y);
    // for (const auto& [id, window] : window_mgr->get_all_windows()) {
    //   if (window == nullptr) {
    //     CORE_LOG_ERROR("Window handle is null, cannot begin frame.");
    //     continue;
    //   }

    //   SDL_GL_MakeCurrent(window, ctx);
    // }

    glm::vec3 clear_color = get_clear_color();
    glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECKGL();
  }

  void opengl_api::on_end_frame(scope<window_manager>& window_mgr) {
    const auto& windows = window_mgr->get_all_windows();
    for (const auto& [id, window] : windows) {
      if (window == nullptr) {
        CORE_LOG_ERROR("Window handle is null, cannot end frame.");
        continue;
      }

      SDL_GL_MakeCurrent(window, gl_ctx(get_gpu_context()));
      SDL_GL_SwapWindow(window);
    }

    CHECKGL();
  }

  void opengl_api::begin_ui_frame_backend_newframe() {
    ImGui_ImplOpenGL3_NewFrame();
  }

  void opengl_api::end_ui_frame_backend_draw_data() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void opengl_api::bind_shader_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t shader_id = itr->second;
    glUseProgram(shader_id);
    CHECKGL();
  }

  void opengl_api::unbind_shader_resource(const resource_handle& handle) {
    glUseProgram(0);

    CHECKGL();
  }

  void opengl_api::compile_and_attach_source(const resource_handle& handle, const std::string& source, shader::source_type type) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot compile shader.");
      return;
    }

    int32_t shader_src_id = -1;
    switch (type) {
      case shader::source_type::VERTEX_SHADER:
        shader_src_id = glCreateShader(GL_VERTEX_SHADER);
        break;

      case shader::source_type::FRAGMENT_SHADER:
        shader_src_id = glCreateShader(GL_FRAGMENT_SHADER);
        break;

      case shader::source_type::COMPUTE_SHADER:
        shader_src_id = glCreateShader(GL_COMPUTE_SHADER);
        break;

      default:
        CORE_LOG_ERROR("Unsupported shader type for OpenGL: {}", static_cast<int>(type));
        return;
    }
    if (shader_src_id <= 0) {
      CORE_LOG_ERROR("Failed to create OpenGL shader source: {}", glGetError());
      return;
    }
    CHECKGL();

    const char* source_cstr = source.c_str();
    glShaderSource(shader_src_id, 1, &source_cstr, nullptr);
    glCompileShader(shader_src_id);
    CHECKGL();

    int success;
    char info_log[512];
    memset(info_log, 0, sizeof(info_log));

    glGetShaderiv(shader_src_id, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader_src_id, 512, nullptr, info_log);
      CORE_LOG_ERROR("Failed to compile OpenGL shader source : {} ({})", info_log, glGetError());
    }
    CHECKGL();

    auto itr = in_process_resources.find(handle.id);
    if (itr == in_process_resources.end()) {
      auto [empl_itr, inserted] = in_process_resources.emplace(handle.id, std::vector<uint32_t>());
      if (!inserted || empl_itr == in_process_resources.end()) {
        CORE_LOG_ERROR("Failed to create in-process resources for shader ID: {}", handle.id);
        return;
      }
      itr = empl_itr;  // Get the iterator to the newly created entry
    }
    auto& in_process = itr->second;
    in_process.push_back(shader_src_id);
  }

  void opengl_api::finalize_shader(const resource_handle& handle) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot finalize shader.");
      return;
    }
    if (handle.type != resource_type::SHADER) {
      CORE_LOG_ERROR("Resource with ID {} is not a shader.", handle.id);
      return;
    }

    /// \todo check resource status
    auto itr = in_process_resources.find(handle.id);
    if (itr == in_process_resources.end()) {
      CORE_LOG_ERROR("In-process resources for shader ID {} not found.", handle.id);
      return;
    }

    int32_t shader_id = glCreateProgram();
    if (shader_id <= 0) {
      CORE_LOG_ERROR("Failed to create OpenGL shader program: {}", glGetError());
      return;
    }

    for (auto& shader_src_id : itr->second) {
      glAttachShader(shader_id, shader_src_id);
    }
    glLinkProgram(shader_id);
    CHECKGL();

    int success;
    char info_log[512];
    memset(info_log, 0, sizeof(info_log));
    glGetProgramiv(shader_id, GL_LINK_STATUS, &success);

    if (!success) {
      glGetProgramInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
      CORE_LOG_ERROR("Failed to link OpenGL shader program: {} ({})", info_log, glGetError());
      memset(info_log, 0, sizeof(info_log));
    }
    CHECKGL();

    glValidateProgram(shader_id);
    glGetProgramiv(shader_id, GL_VALIDATE_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
      CORE_LOG_ERROR("OpenGL shader program validation failed: {} ({})", info_log, glGetError());
      memset(info_log, 0, sizeof(info_log));
    }
    CHECKGL();

    for (auto& shader_src_id : itr->second) {
      glDeleteShader(shader_src_id);
    }
    in_process_resources.erase(itr);
    CHECKGL();

    auto [res_itr, inserted] = gpu_resources.emplace(handle.id, shader_id);
    if (!inserted || res_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for shader ID: {}", handle.id);
      glDeleteProgram(shader_id);
      return;
    }

    auto ip_itr = in_process_resources.find(handle.id);
    if (ip_itr != in_process_resources.end()) {
      in_process_resources.erase(ip_itr);
    }
  }

  void opengl_api::dispatch_shader(const resource_handle& handle, const glm::ivec3& group_dims, shader::compute_barrier_type barrier_type) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot dispatch shader.");
      return;
    }

    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", handle.id);
      return;
    }

    glUseProgram(itr->second);
    glDispatchCompute(group_dims.x, group_dims.y, group_dims.z);
    CHECKGL();

    /// \todo make gl-specific barrier mask from barrier_type
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    CHECKGL();

    glUseProgram(0);
    CHECKGL();
  }

  void opengl_api::bind_texture_resource(const resource_handle& handle, uint32_t index) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
    } else {
      auto texture_itr = texture_resources.find(handle.id);
      if (texture_itr == texture_resources.end()) {
        CORE_LOG_ERROR("Texture resource with ID {} not found in texture resources.", handle.id);
        return;
      }

      uint32_t texture_id = itr->second;
      glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), texture_id);
      glActiveTexture(GL_TEXTURE0 + index);  // Activate the texture unit
      CHECKGL();
    }
  }

  void opengl_api::unbind_texture_resource(const resource_handle& handle, uint32_t index) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
    } else {
      auto texture_itr = texture_resources.find(handle.id);
      if (texture_itr == texture_resources.end()) {
        CORE_LOG_ERROR("Texture resource with ID {} not found in texture resources.", handle.id);
        return;
      }

      glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), 0);  // Unbind the texture
      CHECKGL();
    }
  }

  void opengl_api::set_texture_filter(const resource_handle& handle, texture::filter min_filter, texture::filter mag_filter) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    auto texture_itr = texture_resources.find(handle.id);
    if (texture_itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found in texture resources.", handle.id);
      return;
    }

    glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), itr->second);
    glTexParameteri(get_gl_texture_type(texture_itr->second.get_type()), GL_TEXTURE_MIN_FILTER, get_gl_texture_filter(min_filter));
    glTexParameteri(get_gl_texture_type(texture_itr->second.get_type()), GL_TEXTURE_MAG_FILTER, get_gl_texture_filter(mag_filter));
    glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), 0);  // Unbind the texture
    CHECKGL();
  }

  void opengl_api::set_texture_wrap_mode(const resource_handle& handle, texture::wrap wrap_s, texture::wrap wrap_t, texture::wrap wrap_r) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    auto texture_itr = texture_resources.find(handle.id);
    if (texture_itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found in texture resources.", handle.id);
      return;
    }

    glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), itr->second);
    glTexParameteri(get_gl_texture_type(texture_itr->second.get_type()), GL_TEXTURE_WRAP_S, get_gl_texture_wrap_mode(wrap_s));
    glTexParameteri(get_gl_texture_type(texture_itr->second.get_type()), GL_TEXTURE_WRAP_T, get_gl_texture_wrap_mode(wrap_t));
    glTexParameteri(get_gl_texture_type(texture_itr->second.get_type()), GL_TEXTURE_WRAP_R, get_gl_texture_wrap_mode(wrap_r));
    glBindTexture(get_gl_texture_type(texture_itr->second.get_type()), 0);  // Unbind the texture
    CHECKGL();
  }

  void opengl_api::upload_texture(const resource_handle& handle, texture::tex_type type, texture::format format, const glm::ivec2& img_size, void* data, size_t data_size) {
    auto itr = texture_resources.find(handle.id);
    if (itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("GPU resource for texture ID {} not found.", handle.id);
      return;
    }

    uint32_t texture_id = gpu_itr->second;
    // clang-format off
    glBindTexture(get_gl_texture_type(type), texture_id);
    glTexImage2D(get_gl_texture_type(type), 0, get_gl_texture_format(format), img_size.x, img_size.y, 0, get_gl_texture_channel_format(format), get_gl_texture_format_type(format), data);
    glBindTexture(get_gl_texture_type(type), 0);  // Unbind the texture
    // clang-format on
    CHECKGL();
  }

  void opengl_api::bind_texture_as_image(const resource_handle& handle, uint32_t index, bool writable) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    auto texture_itr = texture_resources.find(handle.id);
    if (texture_itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found in texture resources.", handle.id);
      return;
    }

    uint32_t texture_id = itr->second;
    glBindImageTexture(index, texture_id, 0, GL_FALSE, 0, writable ? GL_READ_WRITE : GL_READ_ONLY, get_gl_texture_format(texture_itr->second.get_format()));
    CHECKGL();
  }

  void opengl_api::bind_buffer_resource(const resource_handle& handle, gpu_buffer::buf_type type) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t buffer_id = itr->second;
    if (type == gpu_buffer::buf_type::UNIFORM_BUFFER ||
        type == gpu_buffer::buf_type::STORAGE_BUFFER) {
      glBindBufferBase(get_gl_buffer_type(type), 0, buffer_id);
    } else {
      glBindBuffer(get_gl_buffer_type(type), buffer_id);
    }
    CHECKGL();
  }

  void opengl_api::unbind_buffer_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }
    auto buf_itr = buffer_resources.find(handle.id);
    if (buf_itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found in buffer resources.", handle.id);
      return;
    }

    if (buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::UNIFORM_BUFFER ||
        buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::STORAGE_BUFFER) {
      glBindBufferBase(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0, 0);
    } else {
      glBindBuffer(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0);
    }
    CHECKGL();
  }

  void opengl_api::bind_shader_buffer_resource(const resource_handle& handle, const resource_handle& shader_handle, const std::string& name, uint32_t binding_point, gpu_buffer::buf_type buffer_type) {
    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }
    uint32_t buffer_id = gpu_itr->second;

    gpu_itr = gpu_resources.find(shader_handle.id);
    if (gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader_handle.id);
      return;
    }
    uint32_t shader_id = gpu_itr->second;

    glBindBuffer(get_gl_buffer_type(buffer_type), buffer_id);
    GLuint blockIndex = glGetUniformBlockIndex(shader_id, name.c_str());
    if (blockIndex == 0xffffffff) {
      CORE_LOG_ERROR("Uniform block '{}' not found in shader ID {}.", name, shader_handle.id);
      return;
    }
    glUniformBlockBinding(shader_id, blockIndex, binding_point);
    glBindBufferBase(get_gl_buffer_type(buffer_type), binding_point, buffer_id);
    glBindBuffer(get_gl_buffer_type(buffer_type), 0);
  }

  void opengl_api::buffer_data(const resource_handle& handle, uint32_t binding_point, const void* data, size_t size) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    auto buf_itr = buffer_resources.find(handle.id);
    if (buf_itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found in buffer resources.", handle.id);
      return;
    }
    if (data == nullptr) {
      size = 0;
    }

    uint32_t buffer_id = itr->second;
    glBindBuffer(get_gl_buffer_type(buf_itr->second.get_buffer_type()), buffer_id);
    glBufferData(get_gl_buffer_type(buf_itr->second.get_buffer_type()), size, data, get_gl_buffer_usage(buf_itr->second.get_usage()));
    glBindBuffer(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0);
    CHECKGL();
  }

  void opengl_api::buffer_range(const resource_handle& handle, uint32_t binding_point, size_t start, size_t size, const void* data) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    auto buf_itr = buffer_resources.find(handle.id);
    if (buf_itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found in buffer resources.", handle.id);
      return;
    }
    if (data == nullptr || size == 0) {
      return;  // No data to upload
    }

    uint32_t buffer_id = itr->second;
    if (buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::UNIFORM_BUFFER ||
        buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::STORAGE_BUFFER) {
      glBindBufferRange(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0, buffer_id, start, size);
    } else {
      glBindBuffer(get_gl_buffer_type(buf_itr->second.get_buffer_type()), buffer_id);
    }
    glBufferSubData(get_gl_buffer_type(buf_itr->second.get_buffer_type()), start, size, data);
    if (buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::UNIFORM_BUFFER ||
        buf_itr->second.get_buffer_type() == gpu_buffer::buf_type::STORAGE_BUFFER) {
      glBindBufferBase(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0, buffer_id);
    } else {
      glBindBuffer(get_gl_buffer_type(buf_itr->second.get_buffer_type()), 0);
    }
    CHECKGL();
  }

  void opengl_api::bind_mesh_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Mesh resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t mesh_id = itr->second;
    glBindVertexArray(mesh_id);
    CHECKGL();
  }

  void opengl_api::unbind_mesh_resource(const resource_handle& handle) {
    glBindVertexArray(0);
  }

  void opengl_api::set_mesh_vertex_attributes(const resource_handle& handle, const std::vector<vertex_attribute>& attributes) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Mesh resource with ID {} not found.", handle.id);
      return;
    }

    size_t stride = 0;
    for (const auto& attr : attributes) {
      stride += attr.size;
    }

    uint32_t mesh_id = itr->second;
    glBindVertexArray(mesh_id);

    size_t offset = 0;
    for (const auto& attr : attributes) {
      size_t full_stride = stride * get_gl_attr_size(mesh::FLOAT);
      size_t full_offset = offset * get_gl_attr_size(mesh::FLOAT);

      glEnableVertexAttribArray(attr.idx);
      glVertexAttribPointer(attr.idx, attr.size, get_gl_attr_type(mesh::FLOAT), GL_FALSE, full_stride, (void*)full_offset);

      offset += attr.size;

      CHECKGL();
    }

    glBindVertexArray(0);
    CHECKGL();
  }

  void opengl_api::draw_mesh(const resource_handle& handle, mesh::primitive_type prim_type, size_t vertex_count, size_t index_count, mesh::attribute_type index_type) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Mesh resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t mesh_id = itr->second;
    glBindVertexArray(mesh_id);

    if (index_count > 0) {
      glDrawElements(get_gl_prim_type(prim_type), index_count, GL_UNSIGNED_INT, nullptr);
    } else {
      glDrawArrays(get_gl_prim_type(prim_type), 0, vertex_count);
    }
    CHECKGL();

    glBindVertexArray(0);
    CHECKGL();
  }

  void opengl_api::bind_framebuffer_resource(const resource_handle& handle) {
    auto itr = framebuffer_resources.find(handle.id);
    if (itr == framebuffer_resources.end()) {
      CORE_LOG_ERROR("Framebuffer resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t framebuffer_id = gpu_resources[handle.id];
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    CHECKGL();

    if (!itr->second.complete) {
      return;
    }

    // Set viewport to match framebuffer size
    const auto& fb = itr->second;
    glViewport(0, 0, fb.size.x, fb.size.y);
    CHECKGL();

    uint32_t clear_flags = 0;

    if (fb.attachment_textures[framebuffer::attachment_type::STENCIL].has_value()) {
      clear_flags |= GL_STENCIL_BUFFER_BIT;
    }
    if (fb.attachment_textures[framebuffer::attachment_type::DEPTH].has_value()) {
      clear_flags |= GL_DEPTH_BUFFER_BIT;
    }

    if (fb.attachment_textures[framebuffer::attachment_type::COLOR].has_value()) {
      clear_flags |= GL_COLOR_BUFFER_BIT;
    }
    OTHER_ASSERT(clear_flags != 0, "Framebuffer must have at least one attachment to clear.");
    glClearColor(fb.clear_color.r, fb.clear_color.g, fb.clear_color.b, fb.clear_color.a);
    CHECKGL();

    glClear(clear_flags);
    CHECKGL();
  }

  void opengl_api::unbind_framebuffer_resource(const resource_handle& handle) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKGL();
  }

  void opengl_api::framebuffer_texture_2d(const resource_handle& handle, const resource_handle& texture, framebuffer::attachment_type type, uint32_t mip_level) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot bind framebuffer texture.");
      return;
    }

    auto gpu_itr = gpu_resources.find(texture.id);
    if (gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Framebuffer resource with ID {} not found.", handle.id);
      return;
    }

    bind_framebuffer_resource(handle);
    CHECKGL();

    glFramebufferTexture2D(GL_FRAMEBUFFER, get_gl_fb_attachment_type(type), GL_TEXTURE_2D, gpu_itr->second, mip_level);
    CHECKGL();

    unbind_framebuffer_resource(handle);
    CHECKGL();
  }

  void opengl_api::finalize_framebuffer(const resource_handle& handle) {
    auto itr = framebuffer_resources.find(handle.id);
    if (itr == framebuffer_resources.end()) {
      CORE_LOG_ERROR("Framebuffer resource with ID {} not found.", handle.id);
      return;
    }

    if (!itr->second.ready_to_finalize) {
      CORE_LOG_ERROR("Framebuffer with ID {} is not complete.", handle.id);
      return;
    }

    uint32_t framebuffer_id = gpu_resources[handle.id];
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

    /// create the renderbuffer now
    uint32_t renderbuffer_id = 0;
    glGenRenderbuffers(1, &renderbuffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, itr->second.size.x, itr->second.size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CHECKGL();

    auto [rb_itr, rb_inserted] = framebuffer_renderbuffers.emplace(handle.id, renderbuffer_id);
    if (!rb_inserted || rb_itr == framebuffer_renderbuffers.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for framebuffer renderbuffer ID: {}", handle.id);

      glDeleteRenderbuffers(1, &renderbuffer_id);
      return;
    }

    // Check if the framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      CORE_LOG_ERROR("Framebuffer with ID {} is not complete: {}", handle.id, status);
      return;
    }
    CHECKGL();
    itr->second.complete = true;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKGL();
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, int32_t value) {
    auto itr = gpu_resources.find(shader.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return;
    }

    uint32_t shader_id = get_shader_uniform_location(shader, name);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Failed to get uniform location for '{}' in shader with ID {}.", name, shader.id);
      return;
    }

    glUseProgram(itr->second);
    glUniform1i(shader_id, value);
    glUseProgram(0);
    CHECKGL();
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, real_t value) {
    auto itr = gpu_resources.find(shader.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return;
    }

    uint32_t shader_id = get_shader_uniform_location(shader, name);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Failed to get uniform location for '{}' in shader with ID {}.", name, shader.id);
      return;
    }

    glUseProgram(itr->second);
    glUniform1f(shader_id, value);
    glUseProgram(0);
    CHECKGL();
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec3& value) {
    auto itr = gpu_resources.find(shader.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return;
    }

    uint32_t shader_id = get_shader_uniform_location(shader, name);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Failed to get uniform location for '{}' in shader with ID {}.", name, shader.id);
      return;
    }

    glUseProgram(itr->second);
    glUniform3fv(shader_id, 1, glm::value_ptr(value));
    glUseProgram(0);
    CHECKGL();
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::vec4& value) {
    auto itr = gpu_resources.find(shader.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return;
    }

    uint32_t shader_id = get_shader_uniform_location(shader, name);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Failed to get uniform location for '{}' in shader with ID {}.", name, shader.id);
      return;
    }

    glUseProgram(itr->second);
    glUniform4fv(shader_id, 1, glm::value_ptr(value));
    glUseProgram(0);
    CHECKGL();
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value, bool transpose) {
    auto itr = gpu_resources.find(shader.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return;
    }

    uint32_t shader_id = get_shader_uniform_location(shader, name);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Failed to get uniform location for '{}' in shader with ID {}.", name, shader.id);
      return;
    }

    glUseProgram(itr->second);
    glUniformMatrix4fv(shader_id, 1, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(value));
    glUseProgram(0);
    CHECKGL();
  }

  int32_t opengl_api::get_gpu_api_window_flags() const {
    return SDL_WINDOW_OPENGL;
  }

  framebuffer* opengl_api::create_framebuffer_resource(const resource_handle& handle, resource_type type) {
    auto itr = framebuffer_resources.find(handle.id);
    if (itr != framebuffer_resources.end()) {
      return &itr->second;
    }

    auto [itr2, inserted] = framebuffer_resources.emplace(handle.id, framebuffer(handle));
    if (!inserted || itr2 == framebuffer_resources.end()) {
      CORE_LOG_ERROR("Failed to create framebuffer resource with ID: {}", handle.id);
      return nullptr;
    }

    uint32_t framebuffer_id = 0;
    glGenFramebuffers(1, &framebuffer_id);
    if (framebuffer_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL framebuffer resource: {}", glGetError());
      framebuffer_resources.erase(itr2);
      return nullptr;
    }
    CHECKGL();

    auto [gpu_itr, gpu_inserted] = gpu_resources.emplace(handle.id, framebuffer_id);
    if (!gpu_inserted || gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for framebuffer ID: {}", handle.id);
      glDeleteFramebuffers(1, &framebuffer_id);
      framebuffer_resources.erase(itr2);
      return nullptr;
    }

    resource_types[handle.id] = type;
    return &itr2->second;
  }

  void opengl_api::destroy_framebuffer_resource(const resource_handle& handle) {
    auto itr = framebuffer_resources.find(handle.id);
    if (itr == framebuffer_resources.end()) {
      CORE_LOG_ERROR("Framebuffer resource with ID {} not found.", handle.id);
      return;
    }

    itr->second.destroy_resources();
    framebuffer_resources.erase(itr);

    auto rb_itr = framebuffer_renderbuffers.find(handle.id);
    if (rb_itr != framebuffer_renderbuffers.end()) {
      uint32_t renderbuffer_id = rb_itr->second;
      glDeleteRenderbuffers(1, &renderbuffer_id);
      framebuffer_renderbuffers.erase(rb_itr);
    } else {
      CORE_LOG_ERROR("Renderbuffer resource for framebuffer ID {} not found.", handle.id);
    }

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr != gpu_resources.end()) {
      uint32_t framebuffer_id = gpu_itr->second;
      glDeleteFramebuffers(1, &framebuffer_id);
      gpu_resources.erase(gpu_itr);
    } else {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", handle.id);
    }

    resource_types.erase(handle.id);
    CHECKGL();
  }

  mesh* opengl_api::create_mesh_resource(const resource_handle& handle, resource_type type) {
    auto itr = mesh_resources.find(handle.id);
    if (itr != mesh_resources.end()) {
      return &itr->second;
    }

    auto [itr2, inserted] = mesh_resources.emplace(handle.id, mesh(handle));
    if (!inserted || itr2 == mesh_resources.end()) {
      CORE_LOG_ERROR("Failed to create mesh resource with ID: {}", handle.id);
      return nullptr;
    }

    uint32_t mesh_id = 0;
    glGenVertexArrays(1, &mesh_id);
    if (mesh_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL mesh resource: {}", glGetError());
      mesh_resources.erase(itr2);
      return nullptr;
    }
    CHECKGL();

    auto [gpu_itr, gpu_inserted] = gpu_resources.emplace(handle.id, mesh_id);
    if (!gpu_inserted || gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for mesh ID: {}", handle.id);
      glDeleteVertexArrays(1, &mesh_id);
      mesh_resources.erase(itr2);
      return nullptr;
    }

    CORE_LOG_DEBUG("Created OpenGL mesh resource with ID: {}", handle.id);

    resource_types[handle.id] = type;
    return &itr2->second;
  }

  void opengl_api::destroy_mesh_resource(const resource_handle& handle) {
    auto itr = mesh_resources.find(handle.id);
    if (itr == mesh_resources.end()) {
      CORE_LOG_ERROR("Mesh resource with ID {} not found.", handle.id);
      return;
    }
    itr->second.destroy_resources();
    mesh_resources.erase(itr);

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr != gpu_resources.end()) {
      uint32_t mesh_id = gpu_itr->second;
      glDeleteVertexArrays(1, &mesh_id);
      gpu_resources.erase(gpu_itr);
    } else {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", handle.id);
    }

    resource_types.erase(handle.id);
    CHECKGL();
  }

  gpu_buffer* opengl_api::create_buffer_resource(const resource_handle& handle, resource_type type) {
    auto itr = buffer_resources.find(handle.id);
    if (itr != buffer_resources.end()) {
      return &itr->second;
    }

    auto [itr2, inserted] = buffer_resources.emplace(handle.id, gpu_buffer(handle));
    if (!inserted || itr2 == buffer_resources.end()) {
      CORE_LOG_ERROR("Failed to create buffer resource with ID: {}", handle.id);
      return nullptr;
    }

    uint32_t buffer_id = 0;
    glGenBuffers(1, &buffer_id);
    if (buffer_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL buffer resource: {}", glGetError());
      buffer_resources.erase(itr2);
      return nullptr;
    }
    CHECKGL();

    auto [gpu_itr, gpu_inserted] = gpu_resources.emplace(handle.id, buffer_id);
    if (!gpu_inserted || gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for buffer ID: {}", handle.id);
      glDeleteBuffers(1, &buffer_id);
      buffer_resources.erase(itr2);
      return nullptr;
    }

    resource_types[handle.id] = type;

    return &itr2->second;
  }

  void opengl_api::destroy_buffer_resource(const resource_handle& handle) {
    auto itr = buffer_resources.find(handle.id);
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    buffer_resources.erase(itr);

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr != gpu_resources.end()) {
      uint32_t buffer_id = gpu_itr->second;
      glDeleteBuffers(1, &buffer_id);
      gpu_resources.erase(gpu_itr);
    } else {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", handle.id);
    }

    resource_types.erase(handle.id);
    CHECKGL();
  }

  texture* opengl_api::create_texture_resource(const resource_handle& handle, resource_type type) {
    auto itr = texture_resources.find(handle.id);
    if (itr != texture_resources.end()) {
      return &itr->second;
    }

    auto [itr2, inserted] = texture_resources.emplace(handle.id, texture(handle));
    if (!inserted || itr2 == texture_resources.end()) {
      CORE_LOG_ERROR("Failed to create texture resource with ID: {}", handle.id);
      return nullptr;
    }

    /// create gpu resource here because texture does not have same requirements as shader
    uint32_t texture_id = 0;
    glGenTextures(1, &texture_id);
    if (texture_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL texture resource: {}", glGetError());
      texture_resources.erase(itr2);
      return nullptr;
    }
    CHECKGL();

    auto [gpu_itr, gpu_inserted] = gpu_resources.emplace(handle.id, texture_id);
    if (!gpu_inserted || gpu_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Failed to create GPU resource for texture ID: {}", handle.id);
      glDeleteTextures(1, &texture_id);
      texture_resources.erase(itr2);
      return nullptr;
    }

    resource_types[handle.id] = type;
    return &itr2->second;
  }

  void opengl_api::destroy_texture_resource(const resource_handle& handle) {
    auto itr = texture_resources.find(handle.id);
    if (itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    texture_resources.erase(itr);

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr != gpu_resources.end()) {
      uint32_t texture_id = gpu_itr->second;
      glDeleteTextures(1, &texture_id);
      gpu_resources.erase(gpu_itr);
    } else {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", handle.id);
    }

    resource_types.erase(handle.id);
    CHECKGL();
  }

  shader* opengl_api::create_shader_resource(const resource_handle& handle, resource_type type) {
    auto itr = shader_resources.find(handle.id);
    if (itr != shader_resources.end()) {
      return &itr->second;
    }

    /// \note we don't create the gpu resource here because the shader needs to be compiled and linked first,
    ///      which is done in compile_and_attach_source and finalize_shader methods.
    auto [itr2, inserted] = shader_resources.emplace(handle.id, shader(handle));
    if (!inserted || itr2 == shader_resources.end()) {
      CORE_LOG_ERROR("Failed to create shader resource with ID: {}", handle.id);
      return nullptr;
    }

    resource_types[handle.id] = type;
    return &itr2->second;
  }

  void opengl_api::destroy_shader_resource(const resource_handle& handle) {
    auto itr = shader_resources.find(handle.id);
    if (itr == shader_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", handle.id);
      return;
    }

    shader_resources.erase(itr);

    auto gpu_itr = gpu_resources.find(handle.id);
    if (gpu_itr != gpu_resources.end()) {
      uint32_t shader_id = gpu_itr->second;
      glDeleteProgram(shader_id);
      gpu_resources.erase(gpu_itr);
    } else {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", handle.id);
    }

    resource_types.erase(handle.id);
    CHECKGL();
  }

  int32_t opengl_api::get_gl_texture_type(texture::tex_type type) const {
    switch (type) {
      case texture::tex_type::TEXTURE_1D:
        return GL_TEXTURE_1D;

      case texture::tex_type::TEXTURE_2D:
        return GL_TEXTURE_2D;

      case texture::tex_type::TEXTURE_3D:
        return GL_TEXTURE_3D;

      case texture::tex_type::TEXTURE_CUBE:
        return GL_TEXTURE_CUBE_MAP;

      default:
        CORE_LOG_ERROR("Unsupported texture type: {}", type);
        return -1;  // Invalid type
    }
  }

  int32_t opengl_api::get_gl_texture_format(texture::format format) const {
    switch (format) {
      case texture::format::RGBA32F:
        return GL_RGBA32F;

      case texture::format::RGBA8:
      case texture::format::RGBA8U:
        return GL_RGBA;

      case texture::format::RGB8:
        return GL_RGB8;

      default:
        CORE_LOG_ERROR("Unsupported texture format: {}", format);
        return -1;  // Invalid format
    }
  }

  int32_t opengl_api::get_gl_texture_channel_format(texture::format format) const {
    switch (format) {
      case texture::format::RGBA32F:
      case texture::format::RGBA8:
      case texture::format::RGBA8U:
        return GL_RGBA;

      case texture::format::RGB8:
        return GL_RGB;

      default:
        CORE_LOG_ERROR("Unsupported texture channel format: {}", format);
        return -1;  // Invalid channel format
    }
  }

  int32_t opengl_api::get_gl_texture_format_type(texture::format format) const {
    switch (format) {
      case texture::format::RGBA32F:
        return GL_FLOAT;

      case texture::format::RGBA8U:
      case texture::format::RGBA8:
      case texture::format::RGB8:
        return GL_UNSIGNED_BYTE;

      default:
        CORE_LOG_ERROR("Unsupported texture format type: {}", format);
        return -1;  // Invalid format type
    }
  }

  int32_t opengl_api::get_gl_texture_filter(texture::filter filter) const {
    switch (filter) {
      case texture::filter::NEAREST:
        return GL_NEAREST;

      case texture::filter::LINEAR:
        return GL_LINEAR;

      case texture::filter::NEAREST_MIPMAP_NEAREST:
        return GL_NEAREST_MIPMAP_NEAREST;

      case texture::filter::LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR_MIPMAP_LINEAR;

      default:
        CORE_LOG_ERROR("Unsupported texture filter: {}", filter);
        return -1;  // Invalid filter
    }
  }

  int32_t opengl_api::get_gl_texture_wrap_mode(texture::wrap wrap) const {
    switch (wrap) {
      case texture::wrap::CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;

      case texture::wrap::MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;

      case texture::wrap::REPEAT:
        return GL_REPEAT;

      case texture::wrap::CLAMP_TO_BORDER:
        return GL_CLAMP_TO_BORDER;

      default:
        CORE_LOG_ERROR("Unsupported texture wrap mode: {}", wrap);
        return -1;  // Invalid wrap mode
    }
  }

  int32_t opengl_api::get_gl_buffer_type(gpu_buffer::buf_type type) const {
    switch (type) {
      case gpu_buffer::buf_type::VERTEX_BUFFER:
        return GL_ARRAY_BUFFER;

      case gpu_buffer::buf_type::INDEX_BUFFER:
        return GL_ELEMENT_ARRAY_BUFFER;

      case gpu_buffer::buf_type::UNIFORM_BUFFER:
        return GL_UNIFORM_BUFFER;

      case gpu_buffer::buf_type::STORAGE_BUFFER:
        return GL_SHADER_STORAGE_BUFFER;

      default:
        CORE_LOG_ERROR("Unsupported buffer type: {}", type);
        return -1;  // Invalid type
    }
  }

  int32_t opengl_api::get_gl_buffer_usage(gpu_buffer::usage usage) const {
    switch (usage) {
      case gpu_buffer::usage::STATIC:
        return GL_STATIC_DRAW;

      case gpu_buffer::usage::DYNAMIC:
        return GL_DYNAMIC_DRAW;

      case gpu_buffer::usage::STREAM:
        return GL_STREAM_DRAW;

      default:
        CORE_LOG_ERROR("Unsupported buffer usage: {}", usage);
        return -1;  // Invalid usage
    }
  }

  int32_t opengl_api::get_gl_attr_type(mesh::attribute_type type) const {
    switch (type) {
      case mesh::attribute_type::BYTE:
        return GL_BYTE;
      case mesh::attribute_type::UNSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
      case mesh::attribute_type::SHORT:
        return GL_SHORT;
      case mesh::attribute_type::UNSIGNED_SHORT:
        return GL_UNSIGNED_SHORT;
      case mesh::attribute_type::INT:
        return GL_INT;
      case mesh::attribute_type::UNSIGNED_INT:
        return GL_UNSIGNED_INT;
      case mesh::attribute_type::FLOAT:
        return GL_FLOAT;
      case mesh::attribute_type::DOUBLE:
        return GL_DOUBLE;
      default:
        CORE_LOG_ERROR("Unsupported attribute type: {}", type);
        return -1;  // Invalid type
    }
  }

  int32_t opengl_api::get_gl_attr_size(mesh::attribute_type type) const {
    switch (type) {
      case mesh::attribute_type::BYTE:
        return sizeof(int8_t);
      case mesh::attribute_type::UNSIGNED_BYTE:
        return sizeof(uint8_t);

      case mesh::attribute_type::SHORT:
        return sizeof(int16_t);
      case mesh::attribute_type::UNSIGNED_SHORT:
        return sizeof(uint16_t);

      case mesh::attribute_type::INT:
        return sizeof(int32_t);
      case mesh::attribute_type::UNSIGNED_INT:
        return sizeof(uint32_t);

      case mesh::attribute_type::FLOAT:
        return sizeof(float);
      case mesh::attribute_type::DOUBLE:
        return sizeof(double);

      default:
        CORE_LOG_ERROR("Unsupported attribute type: {}", type);
        return -1;  // Invalid type
    }
  }

  int32_t opengl_api::get_gl_prim_type(mesh::primitive_type type) const {
    switch (type) {
      case mesh::primitive_type::POINTS:
        return GL_POINTS;

      case mesh::primitive_type::LINES:
        return GL_LINES;

      case mesh::primitive_type::LINE_STRIP:
        return GL_LINE_STRIP;

      case mesh::primitive_type::TRIANGLES:
        return GL_TRIANGLES;

      case mesh::primitive_type::TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;

      case mesh::primitive_type::TRIANGLE_FAN:
        return GL_TRIANGLE_FAN;

      default:
        CORE_LOG_ERROR("Unsupported primitive type: {}", type);
        return -1;  // Invalid type
    }
  }

  int32_t opengl_api::get_gl_fb_attachment_type(framebuffer::attachment_type type) const {
    switch (type) {
      case framebuffer::attachment_type::COLOR:
        return GL_COLOR_ATTACHMENT0;

      case framebuffer::attachment_type::DEPTH:
        return GL_DEPTH_ATTACHMENT;

      case framebuffer::attachment_type::STENCIL:
        return GL_STENCIL_ATTACHMENT;

      case framebuffer::attachment_type::DEPTH_STENCIL:
        return GL_DEPTH_STENCIL_ATTACHMENT;

      default:
        CORE_LOG_ERROR("Unsupported framebuffer attachment type: {}", type);
        return -1;
    }
  }

  int32_t opengl_api::get_resource_handle(natural_t id) const {
    auto itr = gpu_resources.find(id);
    if (itr != gpu_resources.end()) {
      return itr->second;
    }

    CORE_LOG_ERROR("Resource handle with ID {} not found.", id);
    return -1;  // Resource not found
  }

  uint32_t opengl_api::get_shader_uniform_location(const resource_handle& shader, const std::string& name) {
    auto key = uniform_key{ shader.id, FNV(name) };
    auto itr = shader_uniforms.find(key);
    if (itr != shader_uniforms.end()) {
      return itr->second;
    }

    auto shader_itr = gpu_resources.find(shader.id);
    if (shader_itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", shader.id);
      return -1;
    }

    uint32_t shader_id = shader_itr->second;
    GLint location = glGetUniformLocation(shader_id, name.c_str());
    CHECKGL();
    if (location == -1) {
      CORE_LOG_ERROR("Uniform '{}' not found in shader with ID {}.", name, shader.id);
      return -1;
    }

    CORE_LOG_DEBUG("Found uniform '{}' in shader with ID {} at location {}", name, shader.id, location);
    shader_uniforms[key] = location;
    return location;
  }

  namespace {

    void check_for_gl_error(const char* func_name, const char* file, int line) {
      GLenum err;

      constexpr static int max_errors = 10;
      int error_count = 0;
      do {
        err = glGetError();
        switch (err) {
          case GL_NO_ERROR:
            break;

          case GL_INVALID_ENUM:
            CORE_LOG_ERROR("OpenGL error in {} at {}:{}: GL_INVALID_ENUM", func_name, file, line);
            break;

          case GL_INVALID_VALUE:
            CORE_LOG_ERROR("OpenGL error in {} at {}:{}: GL_INVALID_VALUE", func_name, file, line);
            break;

          case GL_INVALID_OPERATION:
            CORE_LOG_ERROR("OpenGL error in {} at {}:{}: GL_INVALID_OPERATION", func_name, file, line);
            break;

          case GL_OUT_OF_MEMORY:
            CORE_LOG_ERROR("OpenGL error in {} at {}:{}: GL_OUT_OF_MEMORY", func_name, file, line);
            break;

          case GL_INVALID_FRAMEBUFFER_OPERATION:
            CORE_LOG_ERROR("OpenGL error in {} at {}:{}: GL_INVALID_FRAMEBUFFER_OPERATION", func_name, file, line);
            break;

          default:
            CORE_LOG_ERROR("Unknown OpenGL error code in {} at {}:{}: {}", func_name, file, line, err);
            return;  // Exit on unknown error
        }
        ++error_count;
      } while (err != GL_NO_ERROR && error_count < max_errors);
      if (error_count >= max_errors) {
        assert(false && "Too many OpenGL errors encountered");
      }
    }

  }  // namespace

}  // namespace other