/**
 * \file ui/asset-editor/asset_editor_node.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_EDITOR_NODE_HPP
#define OTHERLIB_UI_ASSET_EDITOR_NODE_HPP

#include "renderer/ui/ui_node.hpp"

#include "asset/asset.hpp"

namespace other {

  class driver;

  namespace ui {

    class asset_editor_node : public ui_node {
     public:
      asset_editor_node(ui_window* window, const std::string_view name, asset::type type);
      ~asset_editor_node() override = default;

      virtual void draw_editor(asset* asset_ptr, float width, float height) = 0;
      virtual bool draw_mini_preview(asset* asset_ptr, float width) = 0;

      virtual glm::vec4 get_signature_color() const = 0;
      asset::type get_asset_type() const { return editor_asset_type; }

     protected:
      asset::type editor_asset_type;

      /// used by both draw_editor() and draw_mini_preview()
      /// [● TYPE_BADGE  asset_name.ext]
      void draw_signature_header(const asset* asset_ptr, float width, bool compact = false);
      void draw_status_bar(const asset* asset_ptr, float width, const char* extra_info = nullptr);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_EDITOR_NODE_HPP