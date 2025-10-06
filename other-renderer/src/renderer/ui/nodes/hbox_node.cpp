/**
 * \file other-renderer/src/renderer/ui/nodes/hbox_node.cpp
 **/
#include "renderer/ui/nodes/hbox_node.hpp"

#include <imgui/imgui.h>

namespace other {

  void hbox_node::on_render_start() {
    /// recalculate child sizes and positions here if needed
    size_t num_children = children.size();
    if (num_children == 0) {
      return;
    }

    float available_width = get_size().x;
    float child_width = available_width / static_cast<float>(num_children);

    for (auto* child : children) {
      child->set_size({ child_width, get_size().y });
    }

    // set_overriding_child_rendering(true);
  }

  void hbox_node::render_node() {
  }

  void hbox_node::override_child_rendering() {
    // for (auto* child : children) {
    //   child->render();
    // }
    // ImGui::BeginHorizontal();
    //
    // ImGui::EndHorizontal();
  }

}  // namespace other