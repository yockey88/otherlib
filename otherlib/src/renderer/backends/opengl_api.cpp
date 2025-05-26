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

#include "core/logger.hpp"
#include "renderer/renderer_resource.hpp"
#include "renderer/shader.hpp"

namespace other {
  namespace {

    SDL_GLContext gl_ctx(void* handle) {
      return static_cast<SDL_GLContext>(handle);
    }

  }  // namespace

  void opengl_api::initialize() {
    if (native_window<SDL_Window*>() == nullptr) {
      CORE_LOG_ERROR("Native window handle is null.");
      return;
    }
    SDL_Window* window = native_window<SDL_Window*>();
    SDL_GLContext gpu_context = SDL_GL_CreateContext(window);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      CORE_LOG_ERROR("Failed to initialize GLAD: {}", SDL_GetError());
      return;
    }

    if (gpu_context == nullptr) {
      CORE_LOG_ERROR("Failed to create OpenGL context: {}", SDL_GetError());
      return;
    }

    // PROFILE_GPU_CONTEXT;

    std::string gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::string gl_renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    CORE_LOG_DEBUG("OpenGL Version: {}", gl_version);
    CORE_LOG_DEBUG("OpenGL Renderer: {}", gl_renderer);
    set_gpu_context(gpu_context);

    CORE_LOG_INFO("OpenGL API initialized successfully.");
  }

  void opengl_api::shutdown() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot shutdown.");
      return;
    }

    for (auto& [id, type] : resource_types) {
      auto itr = gpu_resources.find(id);
      if (itr != gpu_resources.end()) {
        uint32_t resource_id = itr->second;
        switch (type) {
          case resource_type::BUFFER:
            glDeleteBuffers(1, &resource_id);
            break;

          case resource_type::TEXTURE:
            glDeleteTextures(1, &resource_id);
            break;

          case resource_type::SHADER:
            glDeleteProgram(resource_id);
            break;

          default:
            CORE_LOG_ERROR("Unsupported resource type for OpenGL: {}", static_cast<int>(type));
            break;
        }
      }
    }

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

    ImGui_ImplSDL3_InitForOpenGL(native_window<SDL_Window*>(), get_context_handle());
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
      SDL_Window* window = native_window<SDL_Window*>();
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

  void opengl_api::begin_frame() {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot begin frame.");
      return;
    }

    SDL_GLContext ctx = gl_ctx(get_gpu_context());
    if (ctx == nullptr) {
      CORE_LOG_ERROR("OpenGL context is null, cannot begin frame.");
      return;
    }

    // glViewport(0, 0, window->window_size.x, window->window_size.y);
    glm::vec3 clear_color = get_clear_color();
    glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  }

  void opengl_api::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    SDL_GLContext ctx = gl_ctx(get_gpu_context());
    if (ctx == nullptr) {
      CORE_LOG_ERROR("OpenGL context is null, cannot end frame.");
      return;
    }

    SDL_GL_SwapWindow(native_window<SDL_Window*>());
  }

  void opengl_api::bind_shader_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Shader resource with ID {} not found.", handle.id);
      return;
    }

    uint32_t shader_id = itr->second;
    glUseProgram(shader_id);
  }

  void opengl_api::unbind_shader_resource(const resource_handle& handle) {
    glUseProgram(0);  // Unbind the shader program
  }

  void opengl_api::compile_and_attach_source(const resource_handle& handle, const std::string& source, shader::source_type type) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot compile shader.");
      return;
    }

    int32_t shader_id = get_resource_handle(handle.id);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Invalid shader resource ID: {}", handle.id);
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

    glUseProgram(shader_id);

    const char* source_cstr = source.c_str();
    glShaderSource(shader_src_id, 1, &source_cstr, nullptr);
    glCompileShader(shader_src_id);

    int success;
    glGetShaderiv(shader_src_id, GL_COMPILE_STATUS, &success);
    if (!success) {
      char info_log[512];
      glGetShaderInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
      CORE_LOG_ERROR("Failed to compile OpenGL shader: {}", info_log);
      glDeleteShader(shader_id);
      return;
    }

    glAttachShader(shader_id, shader_src_id);
    glDeleteShader(shader_src_id);

    glUseProgram(shader_id);

    auto itr = in_process_resources.find(handle.id);
    if (itr == in_process_resources.end()) {
      CORE_LOG_ERROR("In-process resources for shader ID {} not found.", handle.id);
      return;
    }
    auto& in_process = itr->second;
    in_process.push_back(shader_src_id);
  }

  void opengl_api::finalize_shader(const resource_handle& handle) {
    if (get_gpu_context() == nullptr) {
      CORE_LOG_ERROR("OpenGL context handle is null, cannot finalize shader.");
      return;
    }

    int32_t shader_id = get_resource_handle(handle.id);
    if (shader_id == -1) {
      CORE_LOG_ERROR("Invalid shader resource ID: {}", handle.id);
      return;
    }

    glLinkProgram(shader_id);
    int success;

    glGetProgramiv(shader_id, GL_LINK_STATUS, &success);
    if (!success) {
      char info_log[512];
      glGetProgramInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
      CORE_LOG_ERROR("Failed to link OpenGL shader program: {}", info_log);
      glDeleteProgram(shader_id);
      return;
    }

    glValidateProgram(shader_id);
    glGetProgramiv(shader_id, GL_VALIDATE_STATUS, &success);
    if (!success) {
      char info_log[512];
      glGetProgramInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
      CORE_LOG_ERROR("OpenGL shader program validation failed: {}", info_log);
      glDeleteProgram(shader_id);
      return;
    }

    auto itr = in_process_resources.find(handle.id);
    if (itr == in_process_resources.end()) {
      CORE_LOG_ERROR("In-process resources for shader ID {} not found.", handle.id);
      return;
    }
    for (auto& shader_src_id : itr->second) {
      glDeleteShader(shader_src_id);
    }
    in_process_resources.erase(itr);  // Clear in-process resources for this shader
    CORE_LOG_INFO("OpenGL shader program finalized successfully.");
  }

  void opengl_api::bind_texture_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    // uint32_t texture_id = itr->second;
    /// TODO: how to know what texture unit to bind to?
    // glBindTexture(GL_TEXTURE_2D, texture_id);
  }

  void opengl_api::unbind_texture_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Texture resource with ID {} not found.", handle.id);
      return;
    }

    // uint32_t texture_id = itr->second;
    // glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
  }

  void opengl_api::bind_buffer_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    // uint32_t buffer_id = itr->second;
    // switch (resource_types[handle.id]) {
    //   case resource_type::BUFFER:
    //     glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    //     break;

    //   case resource_type::TEXTURE:
    //     glBindTexture(GL_TEXTURE_2D, buffer_id);
    //     break;

    //   case resource_type::COMPUTE_SHADER:
    //   case resource_type::VERTEX_SHADER:
    //   case resource_type::FRAGMENT_SHADER:
    //     glUseProgram(buffer_id);
    //     break;

    //   default:
    //     CORE_LOG_ERROR("Unsupported resource type for OpenGL: {}", static_cast<int>(resource_types[handle.id]));
    //     break;
    // }
  }

  void opengl_api::unbind_buffer_resource(const resource_handle& handle) {
    auto itr = gpu_resources.find(handle.id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with ID {} not found.", handle.id);
      return;
    }

    // uint32_t buffer_id = itr->second;
    // switch (resource_types[handle.id]) {
    //   case resource_type::BUFFER:
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind the buffer
    //     break;

    //   case resource_type::TEXTURE:
    //     glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
    //     break;

    //   case resource_type::COMPUTE_SHADER:
    //   case resource_type::VERTEX_SHADER:
    //   case resource_type::FRAGMENT_SHADER:
    //     glUseProgram(0);  // Unbind the shader program
    //     break;

    //   default:
    //     CORE_LOG_ERROR("Unsupported resource type for OpenGL: {}", static_cast<int>(resource_types[handle.id]));
    //     break;
    // }
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, int value) {
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
    glUseProgram(0);  // Unbind the shader program
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, float value) {
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
    glUseProgram(0);  // Unbind the shader program
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
    glUseProgram(0);  // Unbind the shader program
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
    glUseProgram(0);  // Unbind the shader program
  }

  void opengl_api::set_shader_uniform(const resource_handle& shader, const std::string& name, const glm::mat4& value) {
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
    glUniformMatrix4fv(shader_id, 1, GL_FALSE, glm::value_ptr(value));
    glUseProgram(0);  // Unbind the shader program
  }

  resource* opengl_api::get_resource(uint64_t id) {
    auto itr = gpu_resources.find(id);
    if (itr == gpu_resources.end()) {
      CORE_LOG_ERROR("GPU resource with ID {} not found.", id);
      return nullptr;
    }

    auto type_itr = resource_types.find(id);
    if (type_itr == resource_types.end()) {
      CORE_LOG_ERROR("Resource type for ID {} not found.", id);
      return nullptr;
    }

    switch (type_itr->second) {
        // case resource_type::BUFFER:
        //   return new buffer_resource(itr->second);

        // case resource_type::TEXTURE:
        //   return new texture_resource(itr->second);

      case resource_type::SHADER: {
        auto shader_itr = shader_resources.find(id);
        if (shader_itr != shader_resources.end()) {
          return &shader_itr->second;
        }
        CORE_LOG_ERROR("Shader resource with ID {} not found in shader resources.", id);
        return nullptr;
      } break;

      default:
        CORE_LOG_ERROR("Unsupported resource type for OpenGL: {}", static_cast<int>(type_itr->second));
        return nullptr;
    }

    return nullptr;
  }

  void* opengl_api::create_buffer_resource(uint64_t id, resource_type type) {
    auto itr = gpu_resources.find(id);
    if (itr != gpu_resources.end()) {
      return &itr->second;
    }

    uint32_t buffer_id = 0;
    glGenBuffers(1, &buffer_id);
    if (buffer_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL buffer resource with ID: {}", id);
      return nullptr;
    }
    gpu_resources[id] = buffer_id;
    resource_types[id] = type;

    return &gpu_resources[id];
  }

  void* opengl_api::create_texture_resource(uint64_t id, resource_type type) {
    auto itr = gpu_resources.find(id);
    if (itr != gpu_resources.end()) {
      return &itr->second;
    }

    uint32_t texture_id = 0;
    glGenTextures(1, &texture_id);
    if (texture_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL texture resource with ID: {}", id);
      return nullptr;
    }
    gpu_resources[id] = texture_id;
    resource_types[id] = type;

    return &gpu_resources[id];
  }

  void* opengl_api::create_shader_resource(uint64_t id, resource_type type) {
    auto itr = gpu_resources.find(id);
    if (itr != gpu_resources.end()) {
      return &itr->second;
    }

    uint32_t shader_id = glCreateProgram();
    if (shader_id == 0) {
      CORE_LOG_ERROR("Failed to create OpenGL shader resource with ID: {}", id);
      return nullptr;
    }

    gpu_resources[id] = shader_id;
    resource_types[id] = type;

    auto [itr2, inserted] = shader_resources.emplace(id, shader(resource_handle(id, type)));
    if (!inserted) {
      CORE_LOG_ERROR("Failed to create shader resource with ID: {}", id);
      return nullptr;
    }

    return &gpu_resources[id];
  }

  int32_t opengl_api::get_resource_handle(uint64_t id) const {
    auto itr = gpu_resources.find(id);
    if (itr != gpu_resources.end()) {
      return itr->second;
    }
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
    if (location == -1) {
      CORE_LOG_ERROR("Uniform '{}' not found in shader with ID {}.", name, shader.id);
      return -1;
    }
    shader_uniforms[key] = location;
    return location;
  }

}  // namespace other