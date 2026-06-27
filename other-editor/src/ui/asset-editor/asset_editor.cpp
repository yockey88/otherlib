/**
 * \file ui/asset-editor/asset_editor.cpp
 **/
#include "ui/asset-editor/asset_editor.hpp"

#include <format>

#include <imgui/imgui.h>

#include "theme/colors.hpp"

namespace other {
  namespace ui {

    asset_editor::asset_editor(event_system& events, driver* driver_ptr, asset_editor_registry& registry, other::asset* asset_ptr)
        : ui_window(&events, asset_ptr ? asset_ptr->load_path.filename().string() : "Asset Editor", true, ImGuiWindowFlags_None),
          driver_ptr(driver_ptr),
          bound_asset(asset_ptr) {
      if (asset_ptr != nullptr) {
        editor_node = registry.create_editor(asset_ptr->asset_type, this);
      }
    }

    void asset_editor::on_render_body() {
      if (bound_asset == nullptr || editor_node == nullptr) {
        ImGui::TextDisabled("No asset bound or no editor available.");
        return;
      }

      ImVec2 avail = ImGui::GetContentRegionAvail();
      editor_node->draw_editor(bound_asset, avail.x, avail.y);
    }

  }  // namespace ui
}  // namespace other