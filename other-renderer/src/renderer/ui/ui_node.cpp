/**
 * \file renderer/ui/ui_node.cpp
 **/
#include "renderer/ui/ui_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "core/logger.hpp"

#include "renderer/ui/ui_window.hpp"

#include "imgui_internal.h"

namespace other {
  namespace detail {

    struct ui_node_render_end_helper {
      ~ui_node_render_end_helper() { ImGui::EndChild(); }
    };

  }  // namespace detail

  void ui_node::refresh() {
    on_refresh();
    for (auto* child : children) {
      OTHER_ASSERT(child != nullptr, "Null child node in UI node {}", node_title);
      child->refresh();
    }
  }

  void ui_node::render() {
    on_prepare_render();
    {
      detail::ui_node_render_end_helper ___ui_node_render_end_helper_instance{};

      bool current_state = state.open;
      if (!ImGui::BeginChild(std::to_string(id).c_str(), ImVec2{ size.x, size.y }, flags, window_flags)) {
        return;
      }

      /// save imgui state
      ImGuiErrorRecoveryState imgui_state{};
      ImGui::ErrorRecoveryStoreState(&imgui_state);

      try {
        refresh(current_state);
        if (!state.open) {
          return;
        }

        on_render_node_header();
        on_render_node_body();
        for (auto* child : children) {
          OTHER_ASSERT(child != nullptr, "Null child node in UI node {}", node_title);
          child->render();
        }
        on_render_node_footer();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception during UI node render: {}", e.what());
        ImGui::ErrorRecoveryTryToRecoverState(&imgui_state);
      }
    }
    on_render_end();
  }

  natural_t ui_node::add_child_node(scope<ui_node>& node) {
    return add_node_to(node /* this-node */);
  }

  event_system& ui_node::events() {
    return containing_window->get_event_system();
  }

  natural_t ui_node::add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern) {
    OTHER_ASSERT(node != nullptr, "Cannot add null child node to UI node {}", node_title);
    if (remaining_search_pattern.empty()) {
      node->containing_window = containing_window;
      children.push_back(node.get());
      node->parent = id;
      return node->id;
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
      return 0;
    }

    return (*itr)->add_node_to(node, pattern_str);
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