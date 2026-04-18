/**
 * \file scripting/binding_descriptor.cpp
 **/
#include "scripting/binding_descriptor.hpp"

namespace other {

  const field_binding_descriptor* component_binding_descriptor::find_field(uint64_t id) const {
    for (const auto& f : fields) {
      if (f.field_id == id) return &f;
    }
    return nullptr;
  }

  const field_binding_descriptor* component_binding_descriptor::find_field(const std::string_view name) const {
    for (const auto& f : fields) {
      if (f.name == name) return &f;
    }
    return nullptr;
  }

  const method_binding_descriptor* component_binding_descriptor::find_method(uint64_t id) const {
    for (const auto& m : methods) {
      if (m.method_id == id) return &m;
    }
    return nullptr;
  }

  const method_binding_descriptor* service_binding_descriptor::find_method(uint64_t id) const {
    for (const auto& m : methods) {
      if (m.method_id == id) return &m;
    }
    return nullptr;
  }

}  // namespace other