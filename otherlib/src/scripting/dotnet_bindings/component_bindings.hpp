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
    native_string native_render_component_get_mesh_name(natural_t object_id);
    void native_render_component_fetch_mesh(natural_t object_id, float* out_vertex_data, int32_t* out_num_vertices, int32_t* out_index_data, int32_t* out_num_indices);
    void native_render_component_upload_mesh(natural_t object_id, native_string name, const float* vertex_data, const int32_t* vertex_count, const int32_t* index_data, const int32_t* index_count);

    /// materials are assets now: scripts assign a .omat path, parameter poking is a
    ///  material-asset edit (deferred with the material editor)
    native_string native_render_component_get_material_path(natural_t object_id);
    void native_render_component_set_material_path(natural_t object_id, native_string path);

    /// clips are assets: scripts assign a .oanim path ("" = fall back to the component's
    ///  embedded clip name); reads return the current asset's load path
    native_string native_animation_component_get_clip_path(natural_t object_id);
    void native_animation_component_set_clip_path(natural_t object_id, native_string path);

    /// physics settings accessors write the AUTHORED component settings; the scene's
    ///  revalidation pass reconciles the live body on the next frame
    uint32_t native_physics_component_get_body_type(natural_t object_id);
    void native_physics_component_set_body_type(natural_t object_id, uint32_t body_type);
    float native_physics_component_get_mass(natural_t object_id);
    void native_physics_component_set_mass(natural_t object_id, float mass);
    uint32_t native_physics_component_get_is_trigger(natural_t object_id);
    void native_physics_component_set_is_trigger(natural_t object_id, uint32_t is_trigger);

    /// runtime body verbs against the live body (kinematic/static bodies warn-and-ignore forces)
    void native_physics_component_get_linear_velocity(natural_t object_id, float* out_velocity);
    void native_physics_component_set_linear_velocity(natural_t object_id, float x, float y, float z);
    void native_physics_component_get_angular_velocity(natural_t object_id, float* out_velocity);
    void native_physics_component_set_angular_velocity(natural_t object_id, float x, float y, float z);
    void native_physics_component_add_force(natural_t object_id, float x, float y, float z);
    void native_physics_component_add_impulse(natural_t object_id, float x, float y, float z);
    void native_physics_component_add_torque(natural_t object_id, float x, float y, float z);

    /// closest-hit raycast against the active scene's physics world; returns 0 on miss
    uint32_t native_physics_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_distance,
                                    natural_t* out_object, float* out_point, float* out_normal, float* out_distance);

    /// audio clips are assets too: scripts assign a .wav/.mp3 path ("" clears)
    native_string native_audio_source_get_clip_path(natural_t object_id);
    void native_audio_source_set_clip_path(natural_t object_id, native_string path);

    /// fire-and-forget + bus control on the audio environment; no-ops when audio is inert
    void native_audio_play_one_shot(native_string path, float x, float y, float z, float volume, float pitch, uint32_t bus);
    void native_audio_set_bus_volume(uint32_t bus, float volume);
    float native_audio_get_bus_volume(uint32_t bus);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_COMPONENT_BINDINGS_HPP