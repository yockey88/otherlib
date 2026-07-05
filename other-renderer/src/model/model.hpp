/**
 * \file model/model.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_HPP
#define OTHER_RENDERER_MODEL_MODEL_HPP

#include "core/defines.hpp"
#include "math/bounding_box.hpp"
#include "serialization/reflection.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "model/animation.hpp"
#include "model/material.hpp"
#include "model/skeleton.hpp"
#include "model/vertex.hpp"
#include "model/vertex_buffer.hpp"

namespace other {

  struct shader;
  class model_source;

  struct model {
    std::string name;

    model_source* source = nullptr;
    skeleton* skel = nullptr;

    ostd::vector<uint32_t> submesh_indices;

    ostd::vector<glm::mat4> bone_matrices;
    std::unordered_map<uint32_t, glm::mat4> local_submesh_transforms;

    mesh_node* get_node_by_name(const std::string& name);
    submesh* get_submesh_by_name(const std::string& name);
  };

}  // namespace other

OTHER_REFLECT(
  other::model,
  field(name, other::attr::serializable("Name")),
  field(submesh_indices, other::attr::serializable("Submesh Indices")))

#endif  // OTHER_RENDERER_MODEL_MODEL_HPP