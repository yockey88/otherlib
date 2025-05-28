/**
 * \file renderer/shader.cpp
 **/
#include "renderer/shader.hpp"

#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  resource_handle shader::create(const std::string_view name, const std::string_view source, source_type type) {
    if (name.empty() || source.empty()) {
      CORE_LOG_ERROR("Shader name or source is empty, cannot create shader.");
      return { 0, resource_type::EMPTY };
    }

    resource_handle handle = subsystem<renderer_backend>::get()->api()->create_resource(std::string{ name }, resource_type::SHADER);
    if (handle.id == 0) {
      CORE_LOG_ERROR("Failed to create shader resource with name: {}", name);
      return { 0, resource_type::EMPTY };
    }

    (*subsystem<renderer_backend>::get()->api()->get_resource_as<shader>(handle))
      .add_source(std::string{ source }, shader::source_type::COMPUTE_SHADER)
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

    CORE_LOG_DEBUG("Attaching shader source [{}], resource-handle = {}", type, handle().id);
    subsystem<renderer_backend>::get()->api()->compile_and_attach_source(handle(), source, type);

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

  shader& shader::set_uniform(const std::string& name, int32_t value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
  }

  shader& shader::set_uniform(const std::string& name, float value) {
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

  shader& shader::set_uniform(const std::string& name, const glm::mat4& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle(), name, value);
    return *this;
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

    {
      bool has_vertex = false;
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
        } else if (source == source_type::FRAGMENT_SHADER) {
          has_fragment = true;
        }
      }

      if (has_vertex && has_fragment) {
        complete = true;
        final_type = source_type::RENDER_SHADER;
      }
    }
  }

}  // namespace other