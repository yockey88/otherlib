/**
 * \file ui/asset_picker.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_PICKER_HPP
#define OTHERLIB_UI_ASSET_PICKER_HPP

#include <string_view>

#include "core/defines.hpp"

#include "asset/asset.hpp"

namespace other {

  class asset_handler;
  class driver;

  namespace ui {
    namespace inspector {

      /// drag payload shared by the asset browser (source) and inspector asset fields (target);
      ///  lives here (not the editor) so otherlib widgets can accept drops
      constexpr const char* kAssetDragDropPayloadType = "OTHER_ASSET_BROWSER_ASSET_DRAG_PAYLOAD";

      struct asset_drag_drop_payload {
        natural_t handler_asset_id = 0;
        asset::type asset_type = asset::EMPTY;
        /// untracked files carry their load path so the drop site can begin the load itself
        char path[512] = {};
      };

      /// asset-id property row: shows the resolved asset name, click opens a searchable
      ///  picker over the assets mount, accepts asset-browser drops, 'x' clears to 0
      bool property_asset_field(const std::string_view label, natural_t& asset_id, asset::type type, asset_handler* handler, driver* drvr);

    }  // namespace inspector
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_PICKER_HPP
