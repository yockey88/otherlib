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

  bool ui_window::render() {
    on_prepare_render();
    {
      detail::ui_window_end_helper ___ui_window_end_helper_instance{};
      bool is_open = state.open;
      if (!ImGui::Begin(title.c_str(), &is_open, window_flags)) {
        return false;
      }

      /// save imgui state
      ImGuiErrorRecoveryState imgui_state{};
      ImGui::ErrorRecoveryStoreState(&imgui_state);

      try {
        refresh(is_open);
        if (!state.open) {
          return false;
        }

        on_render_header();
        on_render_body();
        on_pre_render_nodes();

        auto root_itr = node_map.find(0);
        OTHER_ASSERT(root_itr != node_map.end(), "UI window {} has no root node", title);
        root_itr->second->render();

        on_post_render_nodes();
        on_render_footer();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception during UI window render: {}", e.what());

        ImGui::ErrorRecoveryTryToRecoverState(&imgui_state);
      }
    }
    on_render_end();
    return state.open;
  }

  void ui_window::toggle_open() {
    state.open = true;
    state.just_opened = true;
    state.just_closed = false;
  }

  void ui_window::toggle_close() {
    state.open = false;
    state.just_opened = false;
    state.just_closed = true;
  }

  natural_t ui_window::add_node(scope<ui_node> node) {
    natural_t id = node->id;
    auto [itr, inserted] = node_map.emplace(id, std::move(node));
    OTHER_ASSERT(inserted, "UI node with ID {} already exists in window {}", itr->first, title);

    CORE_LOG_DEBUG("Added UI node with ID {} to window {}", itr->first, title);
    return add_node_to(itr->second);
  }

  natural_t ui_window::add_node(scope<ui_node> node, const std::string_view parent_search_pattern) {
    natural_t id = node->id;
    if (node_map.find(node->id) != node_map.end()) {
      CORE_LOG_ERROR("UI node with ID {} already exists in window {}", id, title);
      return 0;
    }

    auto [itr, inserted] = node_map.emplace(id, std::move(node));
    OTHER_ASSERT(inserted, "UI node with ID {} already exists in window {}", itr->first, title);

    return add_node_to(itr->second, parent_search_pattern);
  }

  natural_t ui_window::add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern) {
    OTHER_ASSERT(node != nullptr, "Cannot add null node to UI window {}", title);
    auto root_itr = node_map.find(0);
    OTHER_ASSERT(root_itr != node_map.end(), "UI window {} has no root node", title);
    return root_itr->second->add_node_to(node, remaining_search_pattern);
  }

  scope<ui_node>& ui_window::get_node(natural_t node_id) {
    auto itr = node_map.find(node_id);
    OTHER_ASSERT(itr != node_map.end(), "UI node with ID {} not found in window {}", node_id, title);
    return itr->second;
  }

  scope<ui_node>& ui_window::get_node_by_name(const std::string_view node_name) {
    for (auto& [id, node] : node_map) {
      if (node->node_title == node_name) {
        return node;
      }
    }
    OTHER_ASSERT(false, "UI node with name '{}' not found in window {}", node_name, title);
    return node_map.begin()->second;  // to satisfy compiler, will never reach here due to assert
  }

  scope<ui_node>& ui_window::get_node_by_search_pattern(const std::string_view search_pattern) {
    if (search_pattern.empty()) {
      OTHER_ASSERT(false, "Search pattern is empty in window {}", title);
    }

    while (true) {
      auto dot_pos = search_pattern.find('.');
      if (dot_pos == std::string_view::npos) {
        return get_node_by_name(search_pattern);
      } else {
        OTHER_ASSERT(false, "UI window::get_node_by_search_pattern with nested patterns is unimplemented in window {}", title);
        // std::string_view current_name = search_pattern.substr(0, dot_pos);
        // std::string_view remaining_pattern = search_pattern.substr(dot_pos + 1);
        // auto& current_node = get_node_by_name(current_name);
        // return current_node.get_node_by_search_pattern(remaining_pattern);
      }
    }
  }

  void ui_window::refresh(bool current_state) {
    if (current_state != state.open) {
      state.open = current_state;
      state.just_closed = !state.open;
      state.just_opened = state.open;
    } else {
      state.just_closed = false;
      state.just_opened = false;
    }

    state.is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    on_refresh();
    for (auto& [id, node] : node_map) {
      node->refresh();
    }
  }

}  // namespace other