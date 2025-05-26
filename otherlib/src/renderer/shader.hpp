/**
 * \file renderer/shader.hpp
 **/
#ifndef OTHERLIB_RENDERER_SHADER_HPP
#define OTHERLIB_RENDERER_SHADER_HPP

#include <string>

#include <glm/glm.hpp>

#include "renderer/renderer_resource.hpp"

namespace other {

  class shader : public resource {
   public:
    enum source_type {
      VERTEX_SHADER,
      FRAGMENT_SHADER,
      COMPUTE_SHADER
    };

    shader(resource_handle handle)
        : resource(handle) {}
    virtual ~shader() = default;

    resource_type type() const override { return resource_type::SHADER; }

    void bind() const;
    void unbind() const;

    shader& add_source(const std::string& source, source_type type);
    shader& finalize_shader();

    void set_uniform(const std::string& name, int value);
    void set_uniform(const std::string& name, float value);
    void set_uniform(const std::string& name, const glm::vec3& value);
    void set_uniform(const std::string& name, const glm::vec4& value);
    void set_uniform(const std::string& name, const glm::mat4& value);

   protected:
    bool complete = false;
    bool compiled = false;

    std::vector<source_type> sources_attached;

    resource_handle handle;

    void check_build_status();
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_SHADER_HPP