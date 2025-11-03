/**
 * \file model/model_importer.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_IMPORTER_HPP
#define OTHER_RENDERER_MODEL_IMPORTER_HPP

#include <cstdint>
#include <stack>

#include "core/defines.hpp"

#include "model/animation.hpp"
#include "model/material.hpp"
#include "model/skeleton.hpp"
#include "model/vertex.hpp"

#include "glm/fwd.hpp"


namespace other {

  struct model_builder {
    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    std::stack<uint32_t> index_stack;

    glm::mat4 global_transform = glm::mat4(1.0f);
    glm::mat4 inverse_global_transform = glm::mat4(1.0f);

    std::vector<vertex> vertices;
    std::vector<index> indices;

    std::vector<submesh> submeshes;
    std::vector<mesh_node> nodes;

    std::vector<material> materials;

    std::vector<bone> bones;
    std::vector<animation> animations;

    std::unordered_map<uint32_t, std::vector<triangle>> triangles;

    bounding_box bounds = bounding_box::empty;

    void dump_model_info() const;
  };

  namespace model_importer {

    model_builder load_model_data(const filepath& file_path);
    model_builder build_model_data(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);

  }  // namespace model_importer
}  // namespace other

#endif  // OTHER_RENDERER_MODEL_IMPORTER_HPP