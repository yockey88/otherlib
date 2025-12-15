/**
 * \file scripting/binding_utils/type_binder.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_UTILS_TYPE_BINDER_HPP
#define OTHERLIB_SCRIPTING_BINDING_UTILS_TYPE_BINDER_HPP

#include <sol/sol.hpp>

#include "serialization/reflection.hpp"

#include "dotnet/host.hpp"
#include "lua/lua_host.hpp"

#include "object/scene_object.hpp"

namespace other {

  template <typename CT>
  class type_binder {
   public:
    struct lua_binding_context {
      sol::state& lua_state;
      sol::usertype<CT> lua_usertype;

      explicit lua_binding_context(sol::state& state, const std::string_view lua_name)
          : lua_state(state), lua_usertype(state.new_usertype<CT>(lua_name)) {}

      lua_binding_context& bind_reflected_serializable_fields() {
        CORE_LOG_DEBUG("Binding reflected serializable fields for type '{}'", get_lua_type_name());
        if constexpr (reflected_type<CT>) {
          refl::util::for_each(refl::reflect<CT>().members, [&](auto member) {
            if constexpr (!refl::descriptor::is_function(member) && refl::descriptor::has_attribute<attr::serializable>(member)) {
              using field_t = std::remove_cvref_t<decltype(member(std::declval<CT&>()))>;
              field_t CT::* field_ptr = member.pointer;
              std::string name = std::string{ member.name };
              lua_usertype.set(name, field_ptr);
              CORE_LOG_TRACE(" - Bound field '{}' of type [{}]", name, typeid(field_t).name());
            }
          });
        }

        if constexpr (has_type_data_handler<CT>) {
          /// ensures bound types are registered in the type database if they can be
          reflection_data& _ = type_data_handler<CT>::get_reflection_data(CT{});
        }

        return *this;
      }
    };

    static void bind_component_dotnet(dotnet_host& dotnet_host) {
      // dotnet_binding_context{ object, component, dotnet_host }
      //   .attach_native_object();
    }

    static lua_binding_context bind_component_lua(sol::state& lua_state, const std::string_view lua_name = get_lua_type_name()) {
      return lua_binding_context{ lua_state, lua_name }.bind_reflected_serializable_fields();
    }

   private:
    static std::string get_lua_type_name() {
      std::string full_name = std::string{ refl::reflect<CT>().name };
      size_t last_scope = full_name.rfind("::");
      if (last_scope != std::string::npos) {
        return full_name.substr(last_scope + 2);
      }
      return full_name;
    }
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_UTILS_TYPE_BINDER_HPP