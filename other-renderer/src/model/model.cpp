/**
 * \file model/model.cpp
 **/
#include "model/model.hpp"

#include <cstdint>

#include <glm/glm.hpp>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "math/bounding_box.hpp"

#include "gpu_resource/mesh.hpp"
#include "gpu_resource/shader.hpp"
#include "model/model_importer.hpp"
#include "model/model_source.hpp"
#include "model/vertex.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  mesh_node* model::get_node_by_name(const std::string& name) {
    for (auto& node : source->nodes) {
      if (node.name == name) {
        return &node;
      }
    }
    return nullptr;
  }

  submesh* model::get_submesh_by_name(const std::string& name) {
    for (auto& sm : source->submeshes) {
      if (sm.name == name) {
        return &sm;
      }
    }
    return nullptr;
  }

  void model::draw() {
    OTHER_ASSERT(source != nullptr, "SOURCE is null!");
    source->draw();
  }

}  // namespace other