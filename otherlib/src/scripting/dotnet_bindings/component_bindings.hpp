/**
 * \file scripting/dotnet_bindings/component_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP

#include "core/defines.hpp"

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    void native_transform_get_world_matrix(natural_t id, float* out_matrix);

    void native_render_component_get_num_vertices(natural_t object_id, int32_t* out_vertex_count);
    void native_render_component_get_num_indices(natural_t object_id, int32_t* out_index_count);
    void native_render_component_get_active_material_id(natural_t object_id, uint64_t* out_material_id);
    native_string native_render_component_get_mesh_name(natural_t object_id);
    native_string native_render_component_get_material_name(natural_t object_id);
    void native_render_component_fetch_mesh(natural_t object_id, float* out_vertex_data, int32_t* out_num_vertices, int32_t* out_index_data, int32_t* out_num_indices);
    void native_render_component_upload_mesh(natural_t object_id, native_string name, const float* vertex_data, const int32_t* vertex_count, const int32_t* index_data, const int32_t* index_count);

    // clang-format off
    void native_material_fetch_material(natural_t object_id, glm::vec3* out_diffuse_color, float* out_diffuse_reflectivity, glm::vec3* out_specular_color, float* out_specular_reflectivity, glm::vec3* out_emissive_color, float* out_emissivity, 
                                        float* out_transparency, float* out_shininess);
    void native_material_upload_material(natural_t object_id, native_string mat_name, const glm::vec3* diffuse_color, const float* diffuse_reflectivity, const glm::vec3* specular_color, const float* specular_reflectivity, const glm::vec3* emissive_color, const float* emissivity,
                                        const float* transparency, const float* shininess);
    // clang-format on

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP