/**
 * \file renderer/render_graph.cpp
 **/
#include "renderer/render_graph.hpp"

namespace other {

  render_pass& render_graph::bind_pass(uint64_t id, const glm::ivec2& size) {
    auto itr = passes.find(id);
    if (itr != passes.end()) {
      if (size != itr->second.size && size != glm::ivec2{ 0, 0 }) {
        itr->second.size = size;
      }
      return itr->second;
    }

    auto& pass = passes[id] = render_pass(this, id);
    pass.id = id;
    if (size != glm::ivec2{ 0, 0 }) {
      pass.size = size;
    }
    return pass;
  }

}  // namespace other