/**
 * \file renderer/shader.cpp
 **/
#include "renderer/shader.hpp"

#include <iostream>

#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  void shader::bind() const {
    subsystem<renderer_backend>::get()->api()->bind_shader_resource(handle);
  }

  void shader::unbind() const {
    subsystem<renderer_backend>::get()->api()->unbind_shader_resource(handle);
  }

  shader& shader::add_source(const std::string& source, source_type type) {
    CORE_LOG_TRACE("add_source({}, {})", source, type);
    if (complete) {
      CORE_LOG_ERROR("Shader already completed, cannot add more sources.");
      return *this;
    }

    if (source.empty()) {
      CORE_LOG_ERROR("Shader source is empty, cannot add.");
      return *this;
    }

    CORE_LOG_DEBUG("adding source : \n{}", source);
    auto& api = subsystem<renderer_backend>::get()->api();
    api->compile_and_attach_source(handle, source, type);
    sources_attached.push_back(type);
    check_build_status();

    return *this;
  }

  shader& shader::finalize_shader() {
    if (compiled) {
      CORE_LOG_ERROR("Shader already compiled, cannot finalize again.");
      return *this;
    }

    check_build_status();
    if (!complete) {
      CORE_LOG_ERROR("Shader is not complete, cannot finalize.");
      return *this;
    }

    auto& api = subsystem<renderer_backend>::get()->api();
    api->finalize_shader(handle);

    return *this;
  }

  void shader::set_uniform(const std::string& name, int value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle, name, value);
  }

  void shader::set_uniform(const std::string& name, float value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle, name, value);
  }

  void shader::set_uniform(const std::string& name, const glm::vec3& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle, name, value);
  }

  void shader::set_uniform(const std::string& name, const glm::vec4& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle, name, value);
  }

  void shader::set_uniform(const std::string& name, const glm::mat4& value) {
    subsystem<renderer_backend>::get()->api()->set_shader_uniform(handle, name, value);
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
            break;            // Only compute shader, no need for vertex/fragment
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
      } else if (has_vertex || has_fragment) {
        CORE_LOG_ERROR("Shader must have both vertex and fragment shaders.");
        return;
      }
    }
  }

}  // namespace other