/**
 * \file renderer/ui/ui_node.cpp
 **/
#include "renderer/ui/ui_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "core/logger.hpp"

namespace other {
  namespace detail {

    struct ui_node_render_end_helper {
      ~ui_node_render_end_helper() { ImGui::EndChild(); }
    };

  }  // namespace detail

  void ui_node::render() {
    detail::ui_node_render_end_helper ___ui_node_render_end_helper_instance{};

    bool current_state = state.open;
    if (!ImGui::BeginChild(std::to_string(id).c_str())) {
      return;
    }
    try {
      refresh(current_state);
      if (!state.open) {
        return;
      }

      on_render_start();
      render_node();
      for (auto* child : children) {
        OTHER_ASSERT(child != nullptr, "Null child node in UI node {}", node_title);
        child->render();
      }
      on_render_end();
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception during UI node render: {}", e.what());
    }
  }

  void ui_node::add_child_node(scope<ui_node>& node) {
    add_node_to(node /* this-node */);
  }

  void ui_node::add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern) {
    OTHER_ASSERT(node != nullptr, "Cannot add null child node to UI node {}", node_title);
    if (remaining_search_pattern.empty()) {
      children.push_back(node.get());
      node->parent = id;
      return;
    }

    // std::string pattern_str(remaining_search_pattern);
    // auto colon = pattern_str.find_first_of(':');
    // /// this is the case when the name is a single name with no colon
    // if (colon == std::string::npos) {
    //   auto itr = std::ranges::find_if(children, [&pattern_str](const ui_node* child) { return child->node_title == pattern_str; });
    //   if (itr != children.end()) {
    //     (*itr)->add_child_node(node);
    //   } else {
    //     CORE_LOG_ERROR("Failed to find child node '{}' under parent node '{}' to add node '{}'", pattern_str, node_title, node->node_title);
    //   }
    //   return;
    // }

    // /// now we have at least one colon, verify the first half is us and continue down
    // std::string current_name = pattern_str.substr(0, colon);
    // OTHER_ASSERT(current_name == node_title, "Mismatched node name '{}' when adding to parent node '{}'", current_name, node_title);

    // if (colon + 1 >= pattern_str.size()) {
    //   CORE_LOG_ERROR("Invalid node search pattern '{}' when adding node '{}'", pattern_str, node->node_title);
    //   return;
    // }

    // std::string next_pattern = pattern_str.substr(colon + 1);
    // /// no check if we have to strip further to find and pass the pattern on, or if the child is expected here
    // auto colon2 = next_pattern.find_first_of(':');

    // std::string next_name = (colon2 == std::string::npos) ? next_pattern : next_pattern.substr(0, colon2);
    // auto itr = std::ranges::find_if(children, [&next_name](const ui_node* child) { return child->node_title == next_name; });
    // if (itr != children.end()) {
    //   (*itr)->add_node_to(node, next_pattern);
    // } else {
    //   CORE_LOG_ERROR("Failed to find child node '{}' under parent node '{}' to add node '{}'", next_name, node_title, node->node_title);
    // }
  }

  void ui_node::refresh(bool current_state) {
    if (current_state != state.open) {
      state.just_opened = current_state;
      state.just_closed = !current_state;
      state.open = current_state;
    } else {
      state.just_opened = false;
      state.just_closed = false;
    }

    state.is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    on_refresh();
  }

}  // namespace other