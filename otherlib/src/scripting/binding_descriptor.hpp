/**
 * \file scripting/binding_descriptor.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP
#define OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include <refl/refl.hpp>

#include "core/defines.hpp"
#include "core/profiler.hpp"
#include "serialization/reflection.hpp"

#include "scene/scene.hpp"

namespace other {

  class scene;

  struct field_binding_descriptor {
    uint64_t field_id = 0;

    std::string name;
    std::string display_name;

    value_type type = value_type::EMPTY_TYPE;

    size_t size = 0;
    size_t offset = 0;
    uint8_t flags = 0;

    using read_fn_t = bool (*)(const void* comp, void* out_val);
    using write_fn_t = bool (*)(void* comp, const void* in_val);
    read_fn_t read_fn = nullptr;
    write_fn_t write_fn = nullptr;
  };

  struct method_binding_descriptor {
    uint64_t method_id = 0;

    std::string name;
    std::string display_name;

    value_type return_type = value_type::EMPTY_TYPE;
    ostd::vector<reflection_data::member::param_desc> param_types;

    using invoke_fn_t = bool (*)(void* comp, const void** params, void* out_return);
    invoke_fn_t invoke_fn = nullptr;
  };

  struct component_binding_descriptor {
    uint64_t component_id = 0;
    std::string name;

    ostd::vector<field_binding_descriptor> fields;
    ostd::vector<method_binding_descriptor> methods;

    using try_get_fn_t = void* (*)(void* scene, uint64_t object_id);
    using add_fn_t = void* (*)(void* scene, uint64_t object_id);
    using remove_fn_t = bool (*)(void* scene, uint64_t object_id);
    using has_fn_t = bool (*)(void* scene, uint64_t object_id);

    try_get_fn_t try_get_fn = nullptr;
    add_fn_t add_fn = nullptr;
    remove_fn_t remove_fn = nullptr;
    has_fn_t has_fn = nullptr;

    const field_binding_descriptor* find_field(uint64_t id) const;
    const field_binding_descriptor* find_field(const std::string_view name) const;
    const method_binding_descriptor* find_method(uint64_t id) const;
  };

  struct service_binding_descriptor {
    uint64_t service_id = 0;
    std::string name;
    ostd::vector<method_binding_descriptor> methods;

    const method_binding_descriptor* find_method(uint64_t id) const;
  };

  template <typename CT, typename MT>
  field_binding_descriptor make_field_descriptor(MT member) {
    using field_t = std::remove_cvref_t<decltype(member(std::declval<CT&>()))>;

    field_binding_descriptor fd;
    fd.field_id = FNV(std::string{ member.name });
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
      field_t CT::* ptr = MT::pointer;
      *static_cast<field_t*>(out) = typed->*ptr;
      return true;
    };

    if (!(fd.flags & reflection_data::READ_ONLY)) {
      fd.write_fn = +[](void* comp, const void* in) -> bool {
        OTHER_ASSERT(comp != nullptr, "Component pointer is null");
        OTHER_ASSERT(in != nullptr, "Input pointer is null");

        auto* typed = static_cast<CT*>(comp);
        field_t CT::* ptr = MT::pointer;
        typed->*ptr = *static_cast<const field_t*>(in);
        return true;
      };
    }

    return fd;
  }

  template <typename CT>
    requires reflected_type<CT>
  component_binding_descriptor make_component_descriptor(const std::string_view name) {
    PROFILE_SECTION("make_component_descriptor");
    component_binding_descriptor desc;
    desc.component_id = FNV(name);
    desc.name = std::string{ name };

    refl::util::for_each(refl::reflect<CT>().members, [&](auto member) {
      if constexpr (!refl::descriptor::is_function(member)) {
        constexpr bool is_native_only = refl::descriptor::has_attribute<attr::native_only>(member);
        if constexpr (is_native_only) {
          // skip native-only members
          return;
        }

        constexpr bool is_serializable = refl::descriptor::has_attribute<attr::serializable>(member);
        if constexpr (is_serializable) {
          desc.fields.push_back(make_field_descriptor<CT, decltype(member)>(member));
        }
      }
    });

    return desc;
  }

  template <typename CT>
  void set_ecs_lifecycle(component_binding_descriptor& desc) {
    desc.try_get_fn = +[](void* s, uint64_t id) -> void* {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      scene* scene_ptr = static_cast<scene*>(s);
      return static_cast<void*>(scene_ptr->try_get_component<CT>(id));
    };
    desc.add_fn = +[](void* s, uint64_t id) -> void* {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      scene* scene_ptr = static_cast<scene*>(s);
      if (scene_ptr->has_component<CT>(id)) {
        return static_cast<void*>(scene_ptr->try_get_component<CT>(id));
      } else {
        return static_cast<void*>(&scene_ptr->add_component<CT>(id));
      }
    };
    desc.remove_fn = +[](void* s, uint64_t id) -> bool {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      scene* scene_ptr = static_cast<scene*>(s);
      if (scene_ptr->has_component<CT>(id)) {
        scene_ptr->remove_component<CT>(id);
        return true;
      } else {
        return false;
      }
    };
    desc.has_fn = +[](void* s, uint64_t id) -> bool {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null");
      scene* scene_ptr = static_cast<scene*>(s);
      return scene_ptr->has_component<CT>(id);
    };
  }

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP