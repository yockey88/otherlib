/**
 * \file ui/asset_editor_node.cpp
 **/
#include "ui/asset_editor_node.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "renderer/ui/colors.hpp"
#include "renderer/ui/unicode.hpp"

#include "asset/asset.hpp"

namespace other {
  namespace ui {

    using namespace colors;

    inline glm::vec4 get_editor_signature(other::asset::type t) {
      switch (t) {
        case other::asset::TEXTURE: return texture_editor::kSignature;
        // case other::asset::MODEL_SOURCE: return model_source_editor::kSignature;
        // case other::asset::MODEL: return model_editor::kSignature;
        // case other::asset::ANIMATION: return animation_editor::kSignature;
        case other::asset::SCRIPT_SOURCE: return script_source_editor::kSignature;
        case other::asset::SCRIPT: return script_editor::kSignature;
        // case other::asset::AUDIO: return audio_editor::kSignature;
        // case other::asset::SCENE: return scene_editor::kSignature;
        // case other::asset::SCENE_OBJECT: return scene_object::kSignature;
        default: return kTextMuted;
      }
    }

    inline glm::vec4 get_editor_header_tint(other::asset::type t) {
      switch (t) {
        case other::asset::TEXTURE: return texture_editor::kHeaderTint;
        // case other::asset::MODEL_SOURCE: return model_source_editor::kHeaderTint;
        // case other::asset::MODEL: return model_editor::kHeaderTint;
        // case other::asset::ANIMATION: return animation_editor::kHeaderTint;
        case other::asset::SCRIPT_SOURCE: return script_source_editor::kHeaderTint;
        case other::asset::SCRIPT: return script_editor::kHeaderTint;
        // case other::asset::AUDIO: return audio_editor::kHeaderTint;
        // case other::asset::SCENE: return scene_editor::kHeaderTint;
        // case other::asset::SCENE_OBJECT: return scene_object::kHeaderTint;
        default: return asset_editor::kHeaderBG;
      }
    }

    inline const char* get_editor_type_badge(other::asset::type t) {
      switch (t) {
        case other::asset::TEXTURE: return "TEXTURE";
        case other::asset::MODEL_SOURCE: return "MODEL SRC";
        case other::asset::MODEL: return "MODEL";
        case other::asset::ANIMATION: return "ANIMATION";
        case other::asset::SCRIPT_SOURCE: return "SCRIPT SRC";
        case other::asset::SCRIPT: return "SCRIPT";
        case other::asset::AUDIO: return "AUDIO";
        case other::asset::SCENE: return "SCENE";
        // case other::asset::SCENE_OBJECT: return "OBJECT";
        default: return "UNKNOWN";
      }
    }

    asset_editor_node::asset_editor_node(ui_window* window, const std::string_view name, other::asset::type type)
        : ui_node(window, name), editor_asset_type(type) {}

    void asset_editor_node::draw_signature_header(const other::asset* asset_ptr, float width, bool compact) {
      using namespace colors;
      ImDrawList* dl = ImGui::GetWindowDrawList();

      glm::vec4 sig = get_signature_color();
      glm::vec4 tint = get_editor_header_tint(editor_asset_type);
      const char* badge = get_editor_type_badge(editor_asset_type);

      float header_h = compact ? 20.f : 28.f;
      float font_size_badge = compact ? 8.f : 9.f;
      float dot_r = compact ? 3.f : 4.f;

      ImVec2 cursor = ImGui::GetCursorScreenPos();
      ImVec2 header_min = cursor;
      ImVec2 header_max = { cursor.x + width, cursor.y + header_h };

      /// tinted header background
      dl->AddRectFilled(header_min, header_max, to_im_col(tint));

      /// signature dot
      float dot_x = header_min.x + 10.f;
      float dot_y = header_min.y + header_h * 0.5f;
      dl->AddCircleFilled({ dot_x, dot_y }, dot_r, to_im_col(sig));

      /// type badge
      float badge_x = dot_x + dot_r + 6.f;
      float badge_y = header_min.y + (header_h - ImGui::GetTextLineHeight()) * 0.5f;

      ImGui::PushStyleColor(ImGuiCol_Text, rgba_to_imvec4(sig));
      dl->AddText(ImGui::GetFont(), font_size_badge, { badge_x, badge_y + (compact ? 1.f : 0.f) }, to_im_col(sig), badge);
      ImGui::PopStyleColor();

      /// asset name (if available)
      if (asset_ptr != nullptr && !asset_ptr->load_path.empty()) {
        std::string name = asset_ptr->load_path.filename().string();
        ImVec2 badge_size = ImGui::CalcTextSize(badge);
        float name_x = badge_x + badge_size.x + 10.f;

        dl->AddText(ImGui::GetFont(), compact ? 9.f : 11.f, { name_x, badge_y }, to_im_col(asset_editor::kText), name.c_str());
      }

      /// bottom border
      dl->AddLine(
        { header_min.x, header_max.y },
        { header_max.x, header_max.y },
        to_im_col(asset_editor::kBorder), 1.f
      );

      ImGui::SetCursorScreenPos({ cursor.x, cursor.y + header_h + 1.f });
    }

    void asset_editor_node::draw_status_bar(const other::asset* asset_ptr, float width, const char* extra_info) {
      using namespace colors;
      ImDrawList* dl = ImGui::GetWindowDrawList();

      constexpr float status_h = 20.f;
      glm::vec4 sig = get_signature_color();

      ImVec2 cursor = ImGui::GetCursorScreenPos();
      ImVec2 bar_min = cursor;
      ImVec2 bar_max = { cursor.x + width, cursor.y + status_h };

      /// background
      dl->AddRectFilled(bar_min, bar_max, to_im_col(asset_editor::kHeaderBG));

      /// top border
      dl->AddLine(bar_min, { bar_max.x, bar_min.y }, to_im_col(asset_editor::kBorder), 1.f);

      /// signature dot + type name
      float x = bar_min.x + 8.f;
      float text_y = bar_min.y + (status_h - ImGui::GetTextLineHeight()) * 0.5f;

      dl->AddCircleFilled({ x + 3.f, bar_min.y + status_h * 0.5f }, 2.5f, to_im_col(sig));
      x += 10.f;

      const char* badge = get_editor_type_badge(editor_asset_type);
      dl->AddText({ x, text_y }, to_im_col(sig), badge);
      x += ImGui::CalcTextSize(badge).x + 8.f;

      /// separator dot
      dl->AddText({ x, text_y }, to_im_col(asset_editor::kTextMuted), std::format("{}", unicode::kMiddleDot).c_str());
      x += 10.f;

      /// asset name
      if (asset_ptr != nullptr && !asset_ptr->load_path.empty()) {
        std::string stem = asset_ptr->load_path.stem().string();
        std::string ext = asset_ptr->load_path.extension().string();

        dl->AddText({ x, text_y }, to_im_col(kAccent), stem.c_str());
        x += ImGui::CalcTextSize(stem.c_str()).x;

        dl->AddText({ x, text_y }, to_im_col(asset_editor::kTextMuted), (" " + ext).c_str());
        x += ImGui::CalcTextSize((" " + ext).c_str()).x;
      }

      /// extra info
      if (extra_info != nullptr) {
        x += 8.f;
        dl->AddText({ x, text_y }, to_im_col(asset_editor::kTextMuted), std::format("{}", unicode::kMiddleDot).c_str());
        x += 10.f;
        dl->AddText({ x, text_y }, to_im_col(asset_editor::kTextMuted), extra_info);
      }

      ImGui::SetCursorScreenPos({ cursor.x, cursor.y + status_h });
    }

  }  // namespace ui
}  // namespace other