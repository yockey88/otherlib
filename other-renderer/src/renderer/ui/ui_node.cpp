/**
 * \file renderer/ui/ui_node.cpp
 **/
#include "renderer/ui/ui_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "core/logger.hpp"

#include "renderer/ui/ui_window.hpp"

namespace other {
  namespace detail {

    struct ui_node_render_end_helper {
      ~ui_node_render_end_helper() { ImGui::EndChild(); }
    };

  }  // namespace detail

  void ui_node::render() {
    detail::ui_node_render_end_helper ___ui_node_render_end_helper_instance{};

    bool current_state = state.open;
    if (!ImGui::BeginChild(std::to_string(id).c_str(), ImVec2{ size.x, size.y }, flags, window_flags)) {
      return;
    }

    try {
      push_themes();
      refresh(current_state);
      if (!state.open) {
        return;
      }

      on_render_start();
      render_node();
      if (state.override_child_rendering) {
        override_child_rendering();
      } else {
        for (auto* child : children) {
          OTHER_ASSERT(child != nullptr, "Null child node in UI node {}", node_title);
          child->render();
        }
      }
      on_render_end();

      pop_themes();
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
      node->containing_window = containing_window;
      children.push_back(node.get());
      node->parent = id;
      return;
    }

    std::string pattern_str(remaining_search_pattern);
    std::string first_search_name;

    auto colon = pattern_str.find_first_of(':');
    if (colon == std::string::npos) {
      first_search_name = pattern_str;
      pattern_str = "";
    } else {
      first_search_name = pattern_str.substr(0, colon);
      pattern_str = pattern_str.substr(colon + 1);
    }

    auto itr = std::find_if(children.begin(), children.end(), [&first_search_name](const ui_node* child) {
      return child->node_title == first_search_name;
    });
    if (itr == children.end()) {
      CORE_LOG_ERROR("No child node found with name '{}' in UI node '{}'", first_search_name, node_title);
      return;
    }

    (*itr)->add_node_to(node, pattern_str);
  }

  void ui_node::trigger_event(const std::string_view name) {
    if (containing_window) {
      containing_window->get_event_system().trigger_event(name);
    }
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