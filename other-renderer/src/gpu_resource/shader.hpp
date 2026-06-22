/**
 * @file gpu_resource/shader.hpp
 */
#ifndef OTHER_RENDERER_GPU_RESOURCE_SHADER_HPP
#define OTHER_RENDERER_GPU_RESOURCE_SHADER_HPP

#include <string>

#include <glm/glm.hpp>

#include "gpu_resource/renderer_resource.hpp"

namespace other {

  struct shader : public resource {
    OTHER_REFLECTABLE(shader);

    enum source_type : uint8_t {
      INVALID = 0,

      VERTEX_SHADER,
      GEOMETRY_SHADER,
      FRAGMENT_SHADER,

      COMPUTE_SHADER,
      RENDER_SHADER,
      RENDER_GEOM_SHADER,

      NUM_SHADERS
    };

    enum compute_barrier_type : uint8_t {
      NONE = 0,
      SHADER_IMAGE_ACCESS = 1 << 0,
      SHADER_STORAGE = 1 << 1,
      UNIFORM_BARRIER = 1 << 2,
      TEXTURE_FETCH = 1 << 3,
      // add more as needed...

      ALL_BARRIER = SHADER_IMAGE_ACCESS | SHADER_STORAGE | UNIFORM_BARRIER | TEXTURE_FETCH,
      NUM_BARRIER_TYPES
    };

    struct setting {
      std::string setting_name;
      std::string value = "";
      void define(std::string& str) const;
    };

    shader() = default;
    shader(resource_handle handle)
        : resource(handle) {}
    virtual ~shader() = default;

    static resource_handle create(const std::string_view name, const filepath& filepath, const std::vector<setting>& settings);
    static resource_handle create(const std::string_view name, const filepath& vertpath, const filepath& fragpath, const std::vector<setting>& settings);
    static resource_handle create(const std::string_view name, const filepath& vertpath, const filepath& geompath, const filepath& fragpath, const std::vector<setting>& settings);
    static resource_handle create(const std::string_view name, const std::string_view source, source_type type);
    static resource_handle create(const std::string_view name, const std::string_view vert_source, const std::string_view frag_source);
    static std::string preprocess_file(const filepath& file, const std::vector<setting>& settings);

    resource_type type() const override { return resource_type::SHADER; }

    shader& bind();
    shader& dispatch(const glm::ivec3& group_dims = { 1, 1, 1 }, compute_barrier_type barrier_type = compute_barrier_type::NONE);

    shader& add_source(const std::string& source, source_type type);

    shader& set_uniform(const std::string& name, int8_t value);
    shader& set_uniform(const std::string& name, uint8_t value);
    shader& set_uniform(const std::string& name, int16_t value);
    shader& set_uniform(const std::string& name, uint16_t value);
    shader& set_uniform(const std::string& name, int32_t value);
    shader& set_uniform(const std::string& name, uint32_t value);
    shader& set_uniform(const std::string& name, int64_t value);
    shader& set_uniform(const std::string& name, uint64_t value);
    shader& set_uniform(const std::string& name, real_t value);
    shader& set_uniform(const std::string& name, const glm::vec3& value);
    shader& set_uniform(const std::string& name, const glm::vec4& value);
    shader& set_uniform(const std::string& name, const glm::mat4& value, bool transpose = false);
    shader& add_setting(const std::string& setting, opt<std::string> value = std::nullopt);

    void unbind();
    void finalize_shader();

    bool complete = false;
    bool compiled = false;

    uint8_t final_type = source_type::INVALID;
    std::vector<source_type> sources_attached;
    std::vector<std::string> sources;

    std::vector<std::string> setting_definitions;

    static resource_handle create_handle(const std::string_view name);

   private:
    void check_build_status();
  };

}  // namespace other

OTHER_REFLECT(
  other::shader::setting,
  field(setting_name, other::attr::serializable()))

OTHER_REFLECT(
  other::shader);

#endif  // OTHER_RENDERER_GPU_RESOURCE_SHADER_HPP