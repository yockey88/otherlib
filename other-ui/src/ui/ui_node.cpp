/**
 * \file renderer/ui/ui_node.cpp
 **/
#include "ui/ui_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "core/logger.hpp"

#include "ui/ui_window.hpp"

#include "imgui_internal.h"

namespace other {

  void ui_node::refresh() {
    on_refresh();
    for (auto child : children) {
      OTHER_ASSERT(containing_window->get_node(child) != nullptr, "Null child node in UI node {}", node_title);
      containing_window->get_node(child)->refresh();
    }
  }

  void ui_node::render() {
    bool current_state = state.open;
    if (!state.open) {
      return;
    }

    on_prepare_render();
    refresh(current_state);
    if (!state.open) {
      on_render_end();
      return;
    }

    {
      /// save imgui state
      ImGuiErrorRecoveryState imgui_state = {};
      ImGui::ErrorRecoveryStoreState(&imgui_state);

      try {
        on_render_node_header();
        on_render_node_body();
        for (auto child : children) {
          OTHER_ASSERT(containing_window->get_node(child) != nullptr, "Null child node in UI node {}", node_title);
          containing_window->get_node(child)->render();
          ImGui::Separator();
        }
        on_render_node_footer();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception during UI node render: {}", e.what());
        ImGui::ErrorRecoveryTryToRecoverState(&imgui_state);
      }
    }

    on_render_end();
  }

  natural_t ui_node::add_child_node(ref<ui_node> node) {
    return add_node_to(node /* this-node */);
  }

  event_system& ui_node::events() {
    return containing_window->get_event_system();
  }

  natural_t ui_node::add_node_to(ref<ui_node> node, const std::string_view remaining_search_pattern) {
    OTHER_ASSERT(node != nullptr, "Cannot add null child node to UI node {}", node_title);
    if (remaining_search_pattern.empty()) {
      node->containing_window = containing_window;
      children.push_back(node->id);
      node->parent = id;

      CORE_LOG_DEBUG("  - Added child UI node with ID {} to parent node {}", node->id, node_title);
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

    auto itr = std::find_if(children.begin(), children.end(), [this, &first_search_name](const natural_t child) {
      return containing_window->get_node(child)->node_title == first_search_name;
    });
    if (itr == children.end()) {
      CORE_LOG_ERROR("No child node found with name '{}' in UI node '{}'", first_search_name, node_title);
      return 0;
    }

    CORE_LOG_DEBUG(" - Traversing to child UI node '{}' in parent node '{}'", first_search_name, node_title);
    return containing_window->get_node(*itr)->add_node_to(node, pattern_str);
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