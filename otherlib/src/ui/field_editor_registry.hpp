/**
 * \file ui/field_editor_registry.hpp
 **/
#ifndef OTHERLIB_UI_FIELD_EDITOR_REGISTRY_HPP
#define OTHERLIB_UI_FIELD_EDITOR_REGISTRY_HPP

#include <functional>

#include "ui/script/script_field_ui.hpp"

namespace other {
  namespace ui {

    using field_editor_fn = std::function<bool(const std::string_view, void*, const field_context&)>;

    class field_editor_registry {
     public:
      void register_editor(type_key key, const std::string_view type_name, field_editor_fn fn, bool override = false);

      template <typename T>
      void register_editor(field_editor_fn fn, bool override = false) {
        register_editor(type_key_of<T>(), get_type_name_safe<T>(), fn, override);
        constexpr value_type vt = get_value_type<T>();
        if constexpr (vt != value_type::USER_TYPE) {
          register_value_editor(vt, std::move(fn), override);
        }
      }

      void register_value_editor(value_type vt, field_editor_fn fn, bool override = false);

      inline bool has(type_key key) const { return by_type.contains(key); }
      inline bool has(value_type vt) const { return by_value_type.contains(vt); }

      bool edit(type_key key, const std::string_view label, void* data, const field_context& ctx) const;
      bool edit(value_type vt, const std::string_view label, void* data, const field_context& ctx) const;

      template <typename T>
      bool edit(const std::string_view label, T& value, const field_context& ctx) const {
        return edit(type_key_of<T>(), label, &value, ctx);
      }
      bool resolve_type_name(const std::string_view type_name, type_key& out_key) const;

     private:
      struct entry {
        std::string type_name;
        field_editor_fn fn;
      };

      std::unordered_map<type_key, entry> by_type;
      std::unordered_map<value_type, field_editor_fn> by_value_type;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_FIELD_EDITOR_REGISTRY_HPP
