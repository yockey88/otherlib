/**
 * \file ui/asset-editor/asset_editor_registry.hpp
 *
 * Factory registry for creating asset editors.  Keyed by asset::type.
 * Supports script overrides via register_script_editor() so custom
 * editors can be registered from Lua/C# at runtime.
 **/
#ifndef OTHERLIB_UI_ASSET_EDITOR_REGISTRY_HPP
#define OTHERLIB_UI_ASSET_EDITOR_REGISTRY_HPP

#include <functional>
#include <unordered_map>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/scope.hpp"

#include "ui/asset-editor/asset_editor_node.hpp"

#include "asset/asset.hpp"

namespace other {
  namespace ui {

    class asset_editor_registry {
     public:
      using editor_factory_fn = std::function<scope<asset_editor_node>(ui_window*)>;

      void register_editor(asset::type type, editor_factory_fn factory);
      void register_script_editor(const std::string_view type_name, editor_factory_fn factory);

      scope<asset_editor_node> create_editor(asset::type type, ui_window* window) const;

      bool has_editor(asset::type type) const;

     private:
      ostd::unordered_map<asset::type, editor_factory_fn> builtin_editors;
      ostd::unordered_map<natural_t, editor_factory_fn> script_editors;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_EDITOR_REGISTRY_HPP