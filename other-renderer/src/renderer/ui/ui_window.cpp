/**
 * \file renderer/ui/ui_window.cpp
 **/
#include "renderer/ui/ui_window.hpp"

#include <algorithm>
#include <stack>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {
  namespace detail {

    struct ui_window_end_helper {
      ~ui_window_end_helper() { ImGui::End(); }
    };

  }  // namespace detail

  ui_window::ui_window(event_system& events, const std::string_view title, bool open, int32_t flags)
      : id(FNV(title)), title(title), window_flags(flags), events(events) {
    /// add root
    {
      auto [itr, inserted] = node_map.emplace(0, make_scope<window_root>(this));
      OTHER_ASSERT(inserted, "UI node with ID {} already exists in window {}", itr->first, title);
      itr->second->parent = 0xFFFFFFFF;
    }

    state.open = open;
    state.just_closed = false;
    state.just_opened = false;
    state.is_focused = false;
  }

  void ui_window::initialize() {
    on_initialize();
  }

  void ui_window::shutdown() {
    on_shutdown();
  }

  void ui_window::render() {
    detail::ui_window_end_helper ___ui_window_end_helper_instance{};
    bool is_open = state.open;
    if (!ImGui::Begin(title.c_str(), &is_open, window_flags)) {
      return;
    }

    /// save imgui state
    ImGuiErrorRecoveryState imgui_state{};
    ImGui::ErrorRecoveryStoreState(&imgui_state);

    try {
      refresh(is_open);
      if (!state.open) {
        return;
      }

      on_render_start();

      auto root_itr = node_map.find(0);
      OTHER_ASSERT(root_itr != node_map.end(), "UI window {} has no root node", title);
      root_itr->second->render();

      on_render_end();
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception during UI window render: {}", e.what());

      ImGui::ErrorRecoveryTryToRecoverState(&imgui_state);
    }
  }

  void ui_window::add_node(scope<ui_node> node) {
    natural_t id = node->id;
    auto [itr, inserted] = node_map.emplace(id, std::move(node));
    OTHER_ASSERT(inserted, "UI node with ID {} already exists in window {}", itr->first, title);

    add_node_to(itr->second /* this-window */);
  }

  void ui_window::add_node(scope<ui_node> node, const std::string_view parent_search_pattern) {
    natural_t id = node->id;
    auto [itr, inserted] = node_map.emplace(id, std::move(node));
    OTHER_ASSERT(inserted, "UI node with ID {} already exists in window {}", itr->first, title);

    add_node_to(itr->second /* this-window */, parent_search_pattern);
  }

  void ui_window::add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern) {
    OTHER_ASSERT(node != nullptr, "Cannot add null node to UI window {}", title);
    if (remaining_search_pattern.empty()) {
      node->parent = 0;
      auto root = node_map.find(0);
      OTHER_ASSERT(root != node_map.end(), "UI window {} has no root node", title);
      root->second->add_child_node(node);
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

    auto itr = std::ranges::find_if(node_map, [&first_search_name](const auto& pair) { return pair.second->node_title == first_search_name; });
    if (itr != node_map.end()) {
      itr->second->add_node_to(node, pattern_str);
    }
  }

  scope<ui_node>& ui_window::get_node(natural_t node_id) {
    auto itr = node_map.find(node_id);
    OTHER_ASSERT(itr != node_map.end(), "UI node with ID {} not found in window {}", node_id, title);
    return itr->second;
  }

  void ui_window::refresh(bool current_state) {
    if (current_state != state.open) {
      state.just_closed = !current_state;
      state.just_opened = current_state;
      state.open = current_state;
    } else {
      state.just_closed = false;
      state.just_opened = false;
    }

    state.is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    on_refresh();
  }

}  // namespace other