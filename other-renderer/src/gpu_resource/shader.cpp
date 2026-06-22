/**
 * \file gpu_resource/shader.cpp
 **/
#include "gpu_resource/shader.hpp"

#include <fstream>

#include "renderer/gpu_structs.hpp"

#define STB_INCLUDE_LINE_GLSL
#define STB_INCLUDE_IMPLEMENTATION
#include <stb/stb_include.h>

#include "core/logger.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  void shader::setting::define(std::string& str) const {
    str.append("#define ");
    str.append(setting_name);
    if (value != "") {
      str.append(" ");
      str.append(value);
    }
    str.append("\n");
  }

  resource_handle shader::create(const std::string_view name, const filepath& filepath, const std::vector<setting>& settings) {
    resource_handle handle = create_handle(name);
    if (handle.id == 0) {
      return { 0, resource_type::EMPTY };
    }

    CORE_LOG_DEBUG("Creating shader resource with name: {}, from file: {}, handle ID: {}", name, filepath.string(), handle.id);
    std::string source = preprocess_file(filepath, settings);
    if (source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess shader source from file: {}", filepath.string());
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(source, source_type::COMPUTE_SHADER)
      .finalize_shader();

    CORE_LOG_DEBUG("Created shader resource with name: {}, handle ID: {}", name, handle.id);
    return handle;
  }

  resource_handle shader::create(const std::string_view name, const filepath& vertpath, const filepath& fragpath, const std::vector<setting>& settings) {
    resource_handle handle = create_handle(name);
    if (handle.id == 0) {
      return { 0, resource_type::EMPTY };
    }

    std::string vert_source = preprocess_file(vertpath, settings);
    if (vert_source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess vertex shader source from file: {}", vertpath.string());
      return { 0, resource_type::EMPTY };
    }

    std::string frag_source = preprocess_file(fragpath, settings);
    if (frag_source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess fragment shader source from file: {}", fragpath.string());
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(vert_source, source_type::VERTEX_SHADER)
      .add_source(frag_source, source_type::FRAGMENT_SHADER)
      .finalize_shader();

    return handle;
  }

  resource_handle shader::create(const std::string_view name, const filepath& vertpath, const filepath& geompath, const filepath& fragpath, const std::vector<setting>& settings) {
    resource_handle handle = create_handle(name);
    if (handle.id == 0) {
      return { 0, resource_type::EMPTY };
    }

    std::string vert_source = preprocess_file(vertpath, settings);
    if (vert_source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess vertex shader source from file: {}", vertpath.string());
      return { 0, resource_type::EMPTY };
    }

    std::string geom_source = preprocess_file(geompath, settings);
    if (geom_source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess geometry shader source from file: {}", geompath.string());
      return { 0, resource_type::EMPTY };
    }

    std::string frag_source = preprocess_file(fragpath, settings);
    if (frag_source.empty()) {
      CORE_LOG_ERROR("Failed to preprocess fragment shader source from file: {}", fragpath.string());
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(vert_source, source_type::VERTEX_SHADER)
      .add_source(geom_source, source_type::GEOMETRY_SHADER)
      .add_source(frag_source, source_type::FRAGMENT_SHADER)
      .finalize_shader();

    return handle;
  }

  resource_handle shader::create(const std::string_view name, const std::string_view source, source_type type) {
    resource_handle handle = create_handle(name);
    if (handle.id == 0) {
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(std::string{ source }, type)
      .finalize_shader();

    CORE_LOG_DEBUG("Created shader resource with name: {}, handle ID: {}", name, handle.id);
    return handle;
  }

  resource_handle shader::create(const std::string_view name, const std::string_view vert_source, const std::string_view frag_source) {
    if (name.empty() || vert_source.empty() || frag_source.empty()) {
      CORE_LOG_ERROR("Shader name or source is empty, cannot create shader.");
      return { 0, resource_type::EMPTY };
    }

    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ name }, resource_type::SHADER);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create shader resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(std::string{ vert_source }, shader::source_type::VERTEX_SHADER)
      .add_source(std::string{ frag_source }, shader::source_type::FRAGMENT_SHADER)
      .finalize_shader();

    CORE_LOG_DEBUG("Created shader resource with name: {}, handle ID: {}", name, handle.id);
    return handle;
  }

  std::string shader::preprocess_file(const filepath& file, const std::vector<setting>& setting_definitions) {
    CORE_LOG_DEBUG(" - Attempting to preprocess shader source from file: {}", file.string());
    std::ifstream file_stream(file);
    if (!file_stream.is_open()) {
      CORE_LOG_ERROR("Failed to open shader file: {}", file.string());
      return {};
    }

    std::string raw_source;
    {
      std::stringstream ss;
      ss << file_stream.rdbuf();
      file_stream.close();
      raw_source = ss.str();
    }

    char error_message[256];
    std::memset(error_message, 0, sizeof(error_message));

    std::string src;
    src.append("#version 460 core\n");

    const auto builtin_settings = std::array{
      shader::setting{ "MAX_OBJECTS", std::to_string(gpu::kMaxObjects) },
      shader::setting{ "MAX_VERTEX_BONE_INFLUENCE", "4" },
      shader::setting{ "MAX_BONES", std::to_string(gpu::kMaxObjects) },
      shader::setting{ "MAX_MATERIALS", std::to_string(gpu::kMaxMaterials) },
      shader::setting{ "MAX_POINT_LIGHTS", std::to_string(gpu::kMaxPointLights) },
      shader::setting{ "MAX_DIRECTION_LIGHTS", std::to_string(gpu::kMaxDirectionalLights) },
      shader::setting{ "POINT_LIGHT_INTENSITY", "1" },
      shader::setting{ "DIST_FACTOR", "1.1f" },
      shader::setting{ "CONSTANT", "1" },
      shader::setting{ "LINEAR", "1" },
      shader::setting{ "QUADRATIC", "0" },
    };

    for (const auto& setting : setting_definitions) {
      setting.define(src);
    }
    for (const auto& setting : builtin_settings) {
      setting.define(src);
    }
    src.append(raw_source);

    std::string dir_path = file.parent_path().string();
    std::string name = file.filename().string();
    CORE_LOG_DEBUG("   Including shader source from directory: {}, name: {}", dir_path, name);

    char* buffer = src.data();
    char* dir_str_buffer = dir_path.data();
    char* name_buffer = name.data();

    char* included_source = stb_include_string(buffer, nullptr, dir_str_buffer, name_buffer, error_message);
    if (included_source == nullptr) {
      CORE_LOG_ERROR("Failed to include shader source: {}", error_message);
      return {};
    }

    std::string res = std::string(included_source);
    free(included_source);

    return res;
  }

  shader& shader::bind() {
    subsystem<renderer_backend>::get()->api()->bind_shader_resource(handle());
    return *this;
  }

  void shader::unbind() {
    subsystem<renderer_backend>::get()->api()->unbind_shader_resource(handle());
  }

  shader& shader::dispatch(const glm::ivec3& group_dims, compute_barrier_type barrier_type) {
    if (!complete) {
      CORE_LOG_ERROR("Shader is not complete, cannot dispatch.");
      return *this;
    }

    if (final_type != source_type::COMPUTE_SHADER) {
      CORE_LOG_ERROR("Shader is not a compute shader, cannot dispatch.");
      return *this;
    }

    subsystem<renderer_backend>::get()->api()->dispatch_shader(handle(), group_dims, barrier_type);
    return *this;
  }

  shader& shader::add_source(const std::string& source, source_type type) {
    if (complete) {
      CORE_LOG_ERROR("Shader already completed, cannot add more sources.");
      return *this;
    }

    if (source.empty()) {
      CORE_LOG_ERROR("Shader source is empty, cannot add.");
      return *this;
    }

    CORE_LOG_DEBUG("      Attaching shader source [{}], resource-handle = {}", type, handle().id);
    subsystem<renderer_backend>::get()->api()->compile_and_attach_source(handle(), source, type);

    sources.push_back(source);
    sources_attached.push_back(type);
    check_build_status();

    return *this;
  }

  void shader::finalize_shader() {
    if (compiled) {
      CORE_LOG_ERROR("Shader already compiled, cannot finalize again.");
      return;
    }

    check_build_status();
    if (!complete) {
      CORE_LOG_ERROR("Shader is not complete, cannot finalize.");
      return;
    }

    if (final_type == source_type::INVALID) {
      CORE_LOG_ERROR("Shader has no valid type, cannot finalize.");
      return;
    }

    subsystem<renderer_backend>::get()->api()->finalize_shader(handle());
  }

  shader& shader::set_uniform(const std::string& name, int8_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, uint8_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, int16_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, uint16_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, int32_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, uint32_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, int64_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, uint64_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, real_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, const glm::vec3& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, const glm::vec4& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, const glm::mat4& value, bool transpose) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value, transpose);
    return *this;
  }

  shader& shader::add_setting(const std::string& setting, opt<std::string> value) {
    std::string defn = "#define " + setting;
    if (value.has_value()) {
      defn += " " + value.value();
    }
    defn += "\n";
    setting_definitions.push_back(defn);
    CORE_LOG_DEBUG("Added shader setting: {}", defn);
    return *this;
  }

  resource_handle shader::create_handle(const std::string_view name) {
    if (name.empty()) {
      CORE_LOG_ERROR("Shader name is empty, cannot create handle.");
      return { 0, resource_type::EMPTY };
    }

    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ name }, resource_type::SHADER);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create shader handle with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    CORE_LOG_DEBUG("Created shader handle with name: {}, ID: {}", name, handle.id);
    return handle;
  }

  void shader::check_build_status() {
    if (sources_attached.empty()) {
      CORE_LOG_ERROR("Shader has no sources attached, cannot check build status.");
      return;
    }
    if (compiled) {
      CORE_LOG_ERROR("Shader already compiled, cannot check build status again.");
      return;
    }

    bool has_vertex = false;
    bool has_geometry = false;
    bool has_fragment = false;
    for (const auto& source : sources_attached) {
      if (source == source_type::COMPUTE_SHADER) {
        if (sources_attached.size() == 1) {
          complete = true;  // Only compute shader, no vertex/fragment
          final_type = source_type::COMPUTE_SHADER;
          break;  // Only compute shader, no need for vertex/fragment
        } else {
          CORE_LOG_ERROR("Compute shader cannot be combined with other shader types.");
          return;
        }
      } else if (source == source_type::VERTEX_SHADER) {
        has_vertex = true;
      } else if (source == source_type::GEOMETRY_SHADER) {
        has_geometry = true;
      } else if (source == source_type::FRAGMENT_SHADER) {
        has_fragment = true;
      } else {
        CORE_LOG_ERROR("Unsupported shader type: {}", source);
        return;
      }
    }

    if (has_vertex && has_fragment) {
      complete = true;
      final_type = has_geometry ? source_type::RENDER_GEOM_SHADER : source_type::RENDER_SHADER;
    }
  }

}  // namespace other