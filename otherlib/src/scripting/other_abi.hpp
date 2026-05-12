/**
 * \file scripting/other_abi.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_OTHER_ABI_HPP
#define OTHERLIB_SCRIPTING_OTHER_ABI_HPP

#include <cstdint>
#include <unordered_map>

#include "dotnet/native_string.hpp"

#include "scripting/binding_descriptor.hpp"

namespace other {

  class driver;

  class binding_registry {
   public:
    void clear();
    void register_component(component_binding_descriptor desc);
    void register_service(service_binding_descriptor desc);
    const component_binding_descriptor* find_component(uint64_t id) const;
    const component_binding_descriptor* find_component(const std::string_view name) const;
    const service_binding_descriptor* find_service(uint64_t id) const;

    const std::unordered_map<uint64_t, component_binding_descriptor>& get_component_descriptors() const { return component_descriptors; }
    const std::unordered_map<uint64_t, service_binding_descriptor>& get_service_descriptors() const { return service_descriptors; }

    driver* get_driver() const { return driver_ptr; }
    void set_driver(driver* drv) { driver_ptr = drv; }

   private:
    driver* driver_ptr = nullptr;

    std::unordered_map<uint64_t, component_binding_descriptor> component_descriptors;
    std::unordered_map<uint64_t, service_binding_descriptor> service_descriptors;
  };

  namespace abi {

    void oe_init_abi(driver* drv);
    void oe_cleanup_abi();
    binding_registry& oe_get_registry();

    bool oe_validate_handle(uint64_t obj_id, uint32_t generation, uint32_t scene_id);

    bool oe_has_component(uint64_t obj_id, uint64_t comp_id);
    bool oe_add_component(uint64_t obj_id, uint64_t comp_id);
    bool oe_remove_component(uint64_t obj_id, uint64_t comp_id);

    bool oe_get_field_bool(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, bool* out);
    bool oe_set_field_bool(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, bool value);

    bool oe_get_field_i32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, int32_t* out);
    bool oe_set_field_i32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, int32_t value);

    bool oe_get_field_u32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, uint32_t* out);
    bool oe_set_field_u32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, uint32_t value);

    bool oe_get_field_i64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, int64_t* out);
    bool oe_set_field_i64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, int64_t value);

    bool oe_get_field_u64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, uint64_t* out);
    bool oe_set_field_u64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, uint64_t value);

    bool oe_get_field_f32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* out);
    bool oe_set_field_f32(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float value);

    bool oe_get_field_f64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, double* out);
    bool oe_set_field_f64(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, double value);

    bool oe_get_field_vec2(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* x, float* y);
    bool oe_set_field_vec2(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float x, float y);

    bool oe_get_field_vec3(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* x, float* y, float* z);
    bool oe_set_field_vec3(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float x, float y, float z);

    bool oe_get_field_vec4(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* x, float* y, float* z, float* w);
    bool oe_set_field_vec4(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float x, float y, float z, float w);

    bool oe_get_field_quat(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* x, float* y, float* z, float* w);
    bool oe_set_field_quat(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float x, float y, float z, float w);

    bool oe_get_field_mat4(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, float* out_16);
    bool oe_set_field_mat4(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, const float* in_16);

    bool oe_get_field_string(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, native_string* out);
    bool oe_set_field_string(uint64_t obj_id, uint64_t comp_id, uint64_t field_id, native_string value);

  }  // namespace abi
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_OTHER_ABI_HPP