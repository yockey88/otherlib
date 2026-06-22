/**
 * \file ui/field_editor_registry.cpp
 **/
#include "ui/field_editor_registry.hpp"

#include "core/fnv.hpp"

#include "renderer/ui/colors.hpp"

#include "ui/inspector_widgets.hpp"

namespace other {
  namespace ui {

    void field_editor_registry::register_editor(type_key key, const std::string_view type_name, field_editor_fn fn, bool override) {
      OTHER_ASSERT(fn != nullptr, "Null field_editor_fn for type '{}'", type_name);
      auto it = by_type.find(key);
      if (it != by_type.end()) {
        OTHER_ASSERT(override || it->second.type_name == type_name, "type_key collision: '{}' and '{}' share key {}", it->second.type_name, type_name, key);
        OTHER_ASSERT(override, "Duplicate field editor for '{}' (pass override=true to replace)", type_name);
        it->second = entry{ std::string{ type_name }, std::move(fn) };
      } else {
        by_type.emplace(key, entry{ std::string{ type_name }, std::move(fn) });
      }
    }

    void field_editor_registry::register_value_editor(value_type vt, field_editor_fn fn, bool override) {
      OTHER_ASSERT(fn != nullptr, "Null field_editor_fn for value_type '{}'", static_cast<int>(vt));
      OTHER_ASSERT(override || by_value_type.find(vt) == by_value_type.end(), "Duplicate field editor for value_type '{}' (pass override=true to replace)", static_cast<int>(vt));
      by_value_type.insert_or_assign(vt, std::move(fn));
    }

    bool field_editor_registry::edit(type_key key, const std::string_view label, void* data, const field_context& ctx) const {
      OTHER_ASSERT(data != nullptr, "edit(type_key) called with null data for '{}'", label);
      auto it = by_type.find(key);
      if (it == by_type.end()) {
        inspector::property_display(label, "<unsupported type>", colors::kTextDisabled);
        return false;
      }
      return it->second.fn(label, data, ctx);
    }

    bool field_editor_registry::edit(value_type vt, const std::string_view label, void* data, const field_context& ctx) const {
      auto it = by_value_type.find(vt);
      if (it == by_value_type.end()) {
        inspector::property_display(label, std::format("<{}>", get_value_type_string_from_type(vt)), colors::kTextDisabled);
        return false;
      }
      return it->second(label, data, ctx);
    }

    bool field_editor_registry::resolve_type_name(const std::string_view type_name, type_key& out_key) const {
      for (const auto& [key, entry] : by_type) {
        if (entry.type_name == type_name) {
          out_key = key;
          return true;
        }
      }
      return false;
    }

  }  // namespace ui
}  // namespace other