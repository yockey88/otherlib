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
    enum source_type : uint8_t {
      INVALID = 0,

      VERTEX_SHADER,
      FRAGMENT_SHADER,

      COMPUTE_SHADER,
      RENDER_SHADER,

      NUM_SHADERS
    };

    enum compute_barrier_type : uint8_t {
      NONE = 0,
      SHADER_IMAGE_ACCESS,
      /// add more here...

      NUM_BARRIER_TYPES
    };

    shader(resource_handle handle)
        : resource(handle) {}
    virtual ~shader() = default;

    static resource_handle create(const std::string_view name, const std::string_view source, source_type type);
    static resource_handle create(const std::string_view name, const std::string_view vert_source, const std::string_view frag_source);

    resource_type type() const override { return resource_type::SHADER; }

    shader& bind();
    void unbind();

    shader& dispatch(const glm::ivec3& group_dims = { 1, 1, 1 }, compute_barrier_type barrier_type = compute_barrier_type::NONE);

    shader& add_source(const std::string& source, source_type type);
    void finalize_shader();

    shader& set_uniform(const std::string& name, int32_t value);
    shader& set_uniform(const std::string& name, float value);
    shader& set_uniform(const std::string& name, const glm::vec3& value);
    shader& set_uniform(const std::string& name, const glm::vec4& value);
    shader& set_uniform(const std::string& name, const glm::mat4& value);

   protected:
    bool complete = false;
    bool compiled = false;

    source_type final_type = source_type::INVALID;
    std::vector<source_type> sources_attached;

    void check_build_status();
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_SHADER_HPP