/**
 * \file scripting/scene_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_SCENE_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_SCENE_BINDINGS_HPP

#include "serialization/reflection.hpp"

#include "scene/scene.hpp"

#include "scripting/binding_descriptor.hpp"

namespace other {

  template <typename CT, typename MemberDesc>
  field_binding_descriptor make_field_descriptor(MemberDesc member) {
    using field_t = std::remove_cvref_t<decltype(member(std::declval<CT&>()))>;

    field_binding_descriptor fd;
    fd.field_id = FNV(std::string_view{ member.name });
    fd.name = std::string{ member.name };
    fd.type = get_value_type<field_t>();
    fd.size = sizeof(field_t);
    fd.offset = reinterpret_cast<size_t>(&(static_cast<CT*>(nullptr)->*member.pointer));

    if constexpr (refl::descriptor::has_attribute<attr::serializable>(member)) {
      const auto& ser = refl::descriptor::get_attribute<attr::serializable>(member);
      if (!ser.display_name.empty()) {
        fd.display_name = std::string{ ser.display_name };
      }
    }

    fd.read_fn = +[](const void* comp, void* out) -> bool {
      OTHER_ASSERT(comp != nullptr, "Component pointer is null");
      OTHER_ASSERT(out != nullptr, "Output pointer is null");

      const auto* typed = static_cast<const CT*>(comp);
      field_t CT::* ptr = MemberDesc::pointer;
      *static_cast<field_t*>(out) = typed->*ptr;
      return true;
    };

    if (!(fd.flags & reflection_data::READ_ONLY)) {
      fd.write_fn = +[](void* comp, const void* in) -> bool {
        OTHER_ASSERT(comp != nullptr, "Component pointer is null");
        OTHER_ASSERT(in != nullptr, "Input pointer is null");

        auto* typed = static_cast<CT*>(comp);
        field_t CT::* ptr = MemberDesc::pointer;
        typed->*ptr = *static_cast<const field_t*>(in);
        return true;
      };
    }

    return fd;
  }

  template <typename CT>
    requires reflected_type<CT>
  component_binding_descriptor make_component_descriptor(const std::string_view name) {
    component_binding_descriptor desc;
    desc.component_id = FNV(name);
    desc.name = std::string{ name };

    /// lifecycle operations are set by the caller or a specialization
    /// because they require scene& which depends on the component storage model

    refl::util::for_each(refl::reflect<CT>().members, [&](auto member) {
      if constexpr (!refl::descriptor::is_function(member)) {
        constexpr bool is_native_only = refl::descriptor::has_attribute<attr::native_only>(member);
        if constexpr (is_native_only) {
          // skip native-only members
          return;
        }

        constexpr bool is_serializable = refl::descriptor::has_attribute<attr::serializable>(member);
        if constexpr (is_serializable) {
          desc.fields.push_back(make_field_descriptor<CT>(member));
        }
      }
    });

    return desc;
  }

  template <typename CT>
  void set_ecs_lifecycle(component_binding_descriptor& desc) {
    desc.try_get_fn = +[](scene* s, uint64_t id) -> void* {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      return static_cast<void*>(s->try_get_component<CT>(id));
    };
    desc.add_fn = +[](scene* s, uint64_t id) -> void* {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      if (s->has_component<CT>(id)) {
        return static_cast<void*>(s->try_get_component<CT>(id));
      } else {
        return static_cast<void*>(&s->add_component<CT>(id));
      }
    };
    desc.remove_fn = +[](scene* s, uint64_t id) -> bool {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      if (s->has_component<CT>(id)) {
        s->remove_component<CT>(id);
        return true;
      } else {
        return false;
      }
    };
    desc.has_fn = +[](scene* s, uint64_t id) -> bool {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      return s->has_component<CT>(id);
    };
  }

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_SCENE_BINDINGS_HPP