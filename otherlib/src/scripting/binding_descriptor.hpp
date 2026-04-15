/**
 * \file scripting/binding_descriptor.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP
#define OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP

#include <cstdint>
#include <string>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

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
    std::vector<reflection_data::member::param_desc> param_types;

    using invoke_fn_t = bool (*)(void* comp, const void** params, void* out_return);
    invoke_fn_t invoke_fn = nullptr;
  };

  struct component_binding_descriptor {
    uint64_t component_id = 0;
    std::string name;

    std::vector<field_binding_descriptor> fields;
    std::vector<method_binding_descriptor> methods;

    using try_get_fn_t = void* (*)(void* scene, uint64_t object_id);
    using add_fn_t = bool (*)(void* scene, uint64_t object_id);
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
    std::vector<method_binding_descriptor> methods;

    const method_binding_descriptor* find_method(uint64_t id) const;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_DESCRIPTOR_HPP