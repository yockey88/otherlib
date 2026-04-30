/**
 * \file ui/asset-editor/asset_editor.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_EDITOR_HPP
#define OTHERLIB_UI_ASSET_EDITOR_HPP

#include "renderer/ui/ui_window.hpp"

#include "ui/asset-editor/asset_editor_node.hpp"
#include "ui/asset-editor/asset_editor_registry.hpp"

#include "asset/asset.hpp"

namespace other {

  class driver;

  namespace ui {

    class asset_editor : public ui_window {
     public:
      /// construct with a specific asset to edit
      asset_editor(event_system& events, driver* driver_ptr, asset_editor_registry& registry, asset* asset_ptr);
      ~asset_editor() override = default;

      asset* get_asset() const { return bound_asset; }
      natural_t get_asset_id() const { return bound_asset ? bound_asset->id : 0; }

     private:
      driver* driver_ptr = nullptr;
      asset* bound_asset = nullptr;
      scope<asset_editor_node> editor_node;

      void on_render_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_EDITOR_HPP