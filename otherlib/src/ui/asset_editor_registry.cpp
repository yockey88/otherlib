/**
 * \file ui/asset_editor_registry.cpp
 **/
#include "ui/asset_editor_registry.hpp"

#include "core/logger.hpp"

namespace other {
  namespace ui {

    void asset_editor_registry::register_editor(asset::type type, editor_factory_fn factory) {
      builtin_editors[type] = std::move(factory);
    }

    void asset_editor_registry::register_script_editor(const std::string_view type_name, editor_factory_fn factory) {
      natural_t hash = FNV(type_name);
      script_editors[hash] = std::move(factory);
      CORE_LOG_DEBUG("Registered script editor override for '{}'", type_name);
    }

    scope<asset_editor_node> asset_editor_registry::create_editor(asset::type type, ui_window* window) const {
      /// script overrides take priority
      /// \todo: script editors need a type-name → asset::type mapping to work
      ///        for now, only builtin editors are used

      auto it = builtin_editors.find(type);
      if (it != builtin_editors.end()) {
        return it->second(window);
      }

      return nullptr;
    }

    bool asset_editor_registry::has_editor(asset::type type) const {
      return builtin_editors.contains(type);
    }

  }  // namespace ui
}  // namespace other