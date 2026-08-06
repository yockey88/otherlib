/**
 * \file scripting/dotnet_bindings/component_bindings.cpp
 **/
#include "scripting/dotnet_bindings/component_bindings.hpp"

#include "thread/thread_safety.hpp"

#include "audio/audio_environment.hpp"

#include "object/animation_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "scripting/dotnet_bindings/scene_bindings.hpp"

namespace other {
  namespace detail {

    ostd::vector<vertex> build_vertex_data(const float* vertex_data, int32_t vertex_count);
    ostd::vector<index> build_index_data(const int32_t* index_data, int32_t index_count);

  }  // namespace detail
  namespace bindings {

    void native_transform_get_world_matrix(natural_t id, float* out_matrix) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_matrix != nullptr, "Output matrix pointer is null.");

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      glm::mat4 world = active_scene->get_world_transform(id);
      std::memcpy(out_matrix, &world[0][0], sizeof(float) * 16);
    }

    void native_render_component_get_num_vertices(natural_t object_id, int32_t* out_vertex_count) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_vertex_count != nullptr, "Output vertex count pointer is null.");

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      if (active_scene->has_component<render_component>(object_id)) {
        render_component* comp = active_scene->get_component<render_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);
        OTHER_ASSERT(comp->obj_model.source != nullptr, "Render component's model source is null for object with ID {}", object_id);
        *out_vertex_count = comp->obj_model.source->get_num_vertices();
      } else {
        *out_vertex_count = 0;
      }
    }

    void native_render_component_get_num_indices(natural_t object_id, int32_t* out_index_count) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_index_count != nullptr, "Output index count pointer is null.");

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      if (active_scene->has_component<render_component>(object_id)) {
        render_component* comp = active_scene->get_component<render_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);
        OTHER_ASSERT(comp->obj_model.source != nullptr, "Render component's model source is null for object with ID {}", object_id);
        *out_index_count = comp->obj_model.source->get_num_indices();
      } else {
        *out_index_count = 0;
      }
    }

    native_string native_render_component_get_mesh_name(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      native_string result;
      if (active_scene->has_component<render_component>(object_id)) {
        render_component* comp = active_scene->get_component<render_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);
        if (comp->obj_model.source != nullptr) {
          result = native_string{ comp->obj_model.source->get_name() };
        } else {
          /// no-op
        }
      } else {
        /// no-op
      }
      return result;
    }

    native_string native_render_component_get_material_path(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      native_string result;
      if (active_scene->has_component<render_component>(object_id)) {
        render_component* comp = active_scene->get_component<render_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);
        if (comp->material_asset_id != 0) {
          driver* d = detail::get_dotnet_native_driver();
          OTHER_ASSERT(d != nullptr, "Driver is null in native_render_component_get_material_path.");
          asset* mat_asset = d->get_asset(comp->material_asset_id);
          if (mat_asset != nullptr) {
            result = native_string{ mat_asset->load_path.generic_string() };
          }
        }
      } else {
        /// no-op
      }
      return result;
    }

    void native_render_component_set_material_path(natural_t object_id, native_string path) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      /// this call is from user script so handle errors gracefully
      if (!active_scene->has_component<render_component>(object_id)) {
        CORE_LOG_ERROR("Object with ID {} does not have a render component, cannot set material.", object_id);
        return;
      }
      render_component* comp = active_scene->get_component<render_component>(object_id);
      OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);

      const std::string path_str = path;
      if (path_str.empty()) {
        comp->material_asset_id = 0;
        comp->last_material_asset_id = 0;
        return;
      }

      driver* d = detail::get_dotnet_native_driver();
      OTHER_ASSERT(d != nullptr, "Driver is null in native_render_component_set_material_path.");
      const natural_t material_id = d->begin_asset_load(filepath{ path_str });
      if (material_id == 0) {
        CORE_LOG_ERROR("Material '{}' could not begin loading for object ID {}.", path_str, object_id);
        return;
      }
      comp->material_asset_id = material_id;
      comp->last_material_asset_id = material_id;
    }

    native_string native_animation_component_get_clip_path(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      native_string result;
      if (active_scene->has_component<animation_component>(object_id)) {
        animation_component* comp = active_scene->get_component<animation_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Animation component not found for object with ID {}", object_id);
        if (comp->animation_asset_id != 0) {
          driver* d = detail::get_dotnet_native_driver();
          OTHER_ASSERT(d != nullptr, "Driver is null in native_animation_component_get_clip_path.");
          asset* clip_asset = d->get_asset(comp->animation_asset_id);
          if (clip_asset != nullptr) {
            result = native_string{ clip_asset->load_path.generic_string() };
          }
        }
      } else {
        /// no-op
      }
      return result;
    }

    void native_animation_component_set_clip_path(natural_t object_id, native_string path) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      /// this call is from user script so handle errors gracefully
      if (!active_scene->has_component<animation_component>(object_id)) {
        CORE_LOG_ERROR("Object with ID {} does not have an animation component, cannot set clip.", object_id);
        return;
      }
      animation_component* comp = active_scene->get_component<animation_component>(object_id);
      OTHER_ASSERT(comp != nullptr, "Animation component not found for object with ID {}", object_id);

      const std::string path_str = path;
      if (path_str.empty()) {
        /// back to embedded-clip lookup (clip_name), or bind pose if that is empty too
        comp->animation_asset_id = 0;
        comp->last_animation_asset_id = 0;
        return;
      }

      driver* d = detail::get_dotnet_native_driver();
      OTHER_ASSERT(d != nullptr, "Driver is null in native_animation_component_set_clip_path.");
      const natural_t clip_id = d->begin_asset_load(filepath{ path_str });
      if (clip_id == 0) {
        CORE_LOG_ERROR("Animation clip '{}' could not begin loading for object ID {}.", path_str, object_id);
        return;
      }
      comp->animation_asset_id = clip_id;
      comp->last_animation_asset_id = clip_id;
    }

    namespace {

      physics_component* physics_component_for(natural_t object_id) {
        scene* active_scene = detail::get_active_scene_checked();
        OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
        if (!active_scene->has_component<physics_component>(object_id)) {
          return nullptr;
        }
        physics_component* comp = active_scene->get_component<physics_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Physics component not found for object with ID {}", object_id);
        return comp;
      }

    }  // namespace

    uint32_t native_physics_component_get_body_type(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      physics_component* comp = physics_component_for(object_id);
      return comp != nullptr ? comp->settings.body_type : physics_body::STATIC;
    }

    void native_physics_component_set_body_type(natural_t object_id, uint32_t body_type) {
      ASSERT_MAIN_THREAD();
      if (physics_component* comp = physics_component_for(object_id); comp != nullptr) {
        comp->settings.body_type = std::min(body_type, static_cast<uint32_t>(physics_body::NUM_BODY_TYPES - 1));
      }
    }

    float native_physics_component_get_mass(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      physics_component* comp = physics_component_for(object_id);
      return comp != nullptr ? comp->settings.mass : 0.f;
    }

    void native_physics_component_set_mass(natural_t object_id, float mass) {
      ASSERT_MAIN_THREAD();
      if (physics_component* comp = physics_component_for(object_id); comp != nullptr) {
        comp->settings.mass = mass;
      }
    }

    uint32_t native_physics_component_get_is_trigger(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      physics_component* comp = physics_component_for(object_id);
      return (comp != nullptr && comp->settings.is_trigger) ? 1u : 0u;
    }

    void native_physics_component_set_is_trigger(natural_t object_id, uint32_t is_trigger) {
      ASSERT_MAIN_THREAD();
      if (physics_component* comp = physics_component_for(object_id); comp != nullptr) {
        comp->settings.is_trigger = (is_trigger != 0);
      }
    }

    namespace {

      /// live body for the object's physics component, or nullptr (missing component/body/world)
      physics_body* physics_body_for(natural_t object_id, physics_world** out_world) {
        *out_world = nullptr;
        scene* active_scene = detail::get_active_scene_checked();
        OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
        physics_world* world = active_scene->physics();
        if (world == nullptr) {
          return nullptr;
        }
        physics_component* comp = physics_component_for(object_id);
        if (comp == nullptr || comp->body == nullptr) {
          return nullptr;
        }
        *out_world = world;
        return comp->body;
      }

    }  // namespace

    void native_physics_component_get_linear_velocity(natural_t object_id, float* out_velocity) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_velocity != nullptr, "Output velocity pointer is null.");
      out_velocity[0] = out_velocity[1] = out_velocity[2] = 0.f;

      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        glm::vec3 v = world->get_linear_velocity(body);
        out_velocity[0] = v.x;
        out_velocity[1] = v.y;
        out_velocity[2] = v.z;
      }
    }

    void native_physics_component_set_linear_velocity(natural_t object_id, float x, float y, float z) {
      ASSERT_MAIN_THREAD();
      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        world->set_linear_velocity(body, { x, y, z });
      }
    }

    void native_physics_component_get_angular_velocity(natural_t object_id, float* out_velocity) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_velocity != nullptr, "Output velocity pointer is null.");
      out_velocity[0] = out_velocity[1] = out_velocity[2] = 0.f;

      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        glm::vec3 v = world->get_angular_velocity(body);
        out_velocity[0] = v.x;
        out_velocity[1] = v.y;
        out_velocity[2] = v.z;
      }
    }

    void native_physics_component_set_angular_velocity(natural_t object_id, float x, float y, float z) {
      ASSERT_MAIN_THREAD();
      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        world->set_angular_velocity(body, { x, y, z });
      }
    }

    void native_physics_component_add_force(natural_t object_id, float x, float y, float z) {
      ASSERT_MAIN_THREAD();
      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        world->add_force(body, { x, y, z });
      }
    }

    void native_physics_component_add_impulse(natural_t object_id, float x, float y, float z) {
      ASSERT_MAIN_THREAD();
      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        world->add_impulse(body, { x, y, z });
      }
    }

    void native_physics_component_add_torque(natural_t object_id, float x, float y, float z) {
      ASSERT_MAIN_THREAD();
      physics_world* world = nullptr;
      if (physics_body* body = physics_body_for(object_id, &world); body != nullptr) {
        world->add_torque(body, { x, y, z });
      }
    }

    uint32_t native_physics_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_distance,
                                    natural_t* out_object, float* out_point, float* out_normal, float* out_distance) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_object != nullptr && out_point != nullptr && out_normal != nullptr && out_distance != nullptr,
                   "Raycast output pointers are null.");
      *out_object = 0;
      *out_distance = 0.f;
      out_point[0] = out_point[1] = out_point[2] = 0.f;
      out_normal[0] = out_normal[1] = out_normal[2] = 0.f;

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
      physics_world* world = active_scene->physics();
      if (world == nullptr) {
        return 0;
      }

      raycast_hit hit = world->cast_ray({ ox, oy, oz }, { dx, dy, dz }, max_distance);
      if (!hit.hit) {
        return 0;
      }

      *out_object = hit.owner_object_id;
      *out_distance = hit.distance;
      out_point[0] = hit.point.x;
      out_point[1] = hit.point.y;
      out_point[2] = hit.point.z;
      out_normal[0] = hit.normal.x;
      out_normal[1] = hit.normal.y;
      out_normal[2] = hit.normal.z;
      return 1;
    }

    native_string native_audio_source_get_clip_path(natural_t object_id) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      native_string result;
      if (active_scene->has_component<audio_source_component>(object_id)) {
        audio_source_component* comp = active_scene->get_component<audio_source_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Audio source component not found for object with ID {}", object_id);
        if (comp->clip_asset_id != 0) {
          driver* d = detail::get_dotnet_native_driver();
          OTHER_ASSERT(d != nullptr, "Driver is null in native_audio_source_get_clip_path.");
          asset* clip_asset = d->get_asset(comp->clip_asset_id);
          if (clip_asset != nullptr) {
            result = native_string{ clip_asset->load_path.generic_string() };
          }
        }
      }
      return result;
    }

    void native_audio_source_set_clip_path(natural_t object_id, native_string path) {
      ASSERT_MAIN_THREAD();
      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      /// this call is from user script so handle errors gracefully
      if (!active_scene->has_component<audio_source_component>(object_id)) {
        CORE_LOG_ERROR("Object with ID {} does not have an audio source component, cannot set clip.", object_id);
        return;
      }
      audio_source_component* comp = active_scene->get_component<audio_source_component>(object_id);
      OTHER_ASSERT(comp != nullptr, "Audio source component not found for object with ID {}", object_id);

      const std::string path_str = path;
      if (path_str.empty()) {
        comp->clip_asset_id = 0;
        return;
      }

      driver* d = detail::get_dotnet_native_driver();
      OTHER_ASSERT(d != nullptr, "Driver is null in native_audio_source_set_clip_path.");
      const natural_t clip_id = d->begin_asset_load(filepath{ path_str });
      if (clip_id == 0) {
        CORE_LOG_ERROR("Audio clip '{}' could not begin loading for object ID {}.", path_str, object_id);
        return;
      }
      comp->clip_asset_id = clip_id;
    }

    void native_audio_play_one_shot(native_string path, float x, float y, float z, float volume, float pitch, uint32_t bus) {
      ASSERT_MAIN_THREAD();
      if (subsystem<audio_environment>::inert) {
        return;
      }
      audio_environment* env = subsystem<audio_environment>::get();
      if (env == nullptr || !env->is_initialized()) {
        return;
      }

      driver* d = detail::get_dotnet_native_driver();
      OTHER_ASSERT(d != nullptr, "Driver is null in native_audio_play_one_shot.");
      const std::string path_str = path;
      const natural_t clip_id = d->begin_asset_load(filepath{ path_str });
      if (clip_id == 0) {
        CORE_LOG_ERROR("Audio clip '{}' could not begin loading for one-shot.", path_str);
        return;
      }
      asset* clip_asset = d->get_asset(clip_id);
      if (clip_asset == nullptr) {
        return;
      }

      voice_params params{};
      params.clip_hash = clip_asset->path_hash;
      params.volume = volume;
      params.pitch = pitch;
      params.bus = static_cast<audio_bus>(std::min<uint32_t>(bus, static_cast<uint32_t>(audio_bus::NUM_BUSES) - 1));
      params.spatial = true;
      params.position = { x, y, z };
      /// if the clip is still mid-load the environment warns and refuses — one-shots
      ///  are best-effort by design, the next call after load lands will sound
      env->play_one_shot(params);
    }

    void native_audio_set_bus_volume(uint32_t bus, float volume) {
      ASSERT_MAIN_THREAD();
      if (subsystem<audio_environment>::inert) {
        return;
      }
      audio_environment* env = subsystem<audio_environment>::get();
      if (env == nullptr) {
        return;
      }
      if (bus >= static_cast<uint32_t>(audio_bus::NUM_BUSES)) {
        CORE_LOG_ERROR("Invalid audio bus {} in SetBusVolume.", bus);
        return;
      }
      env->set_bus_volume(static_cast<audio_bus>(bus), volume);
    }

    float native_audio_get_bus_volume(uint32_t bus) {
      ASSERT_MAIN_THREAD();
      if (subsystem<audio_environment>::inert) {
        return 0.f;
      }
      audio_environment* env = subsystem<audio_environment>::get();
      if (env == nullptr || bus >= static_cast<uint32_t>(audio_bus::NUM_BUSES)) {
        return 0.f;
      }
      return env->bus_volume(static_cast<audio_bus>(bus));
    }

    void native_render_component_fetch_mesh(natural_t object_id, float* out_vertex_data, int32_t* out_num_vertices, int32_t* out_index_data, int32_t* out_num_indices) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(out_vertex_data != nullptr, "Output vertex data pointer is null.");
      OTHER_ASSERT(out_index_data != nullptr, "Output index data pointer is null.");
      OTHER_ASSERT(out_num_vertices != nullptr, "Output vertex count pointer is null.");
      OTHER_ASSERT(out_num_indices != nullptr, "Output index count pointer is null.");

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");

      if (active_scene->has_component<render_component>(object_id)) {
        render_component* comp = active_scene->get_component<render_component>(object_id);
        OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);
        OTHER_ASSERT(comp->obj_model.source != nullptr, "Render component's model source is null for object with ID {}", object_id);

        const std::span<const vertex> vertices = comp->obj_model.source->source_data().vertices;
        const std::span<const index> indices = comp->obj_model.source->source_data().indices;

        *out_num_vertices = static_cast<int32_t>(vertices.size());
        *out_num_indices = static_cast<int32_t>(indices.size());
        std::memcpy(out_vertex_data, vertices.data(), sizeof(vertex) * vertices.size());
        std::memcpy(out_index_data, indices.data(), sizeof(index) * indices.size());
      } else {
        *out_num_vertices = 0;
        *out_num_indices = 0;
      }
    }

    void native_render_component_upload_mesh(natural_t object_id, native_string name, const float* vertex_data, const int32_t* vertex_count, const int32_t* index_data, const int32_t* index_count) {
      ASSERT_MAIN_THREAD();
      OTHER_ASSERT(vertex_data != nullptr, "Vertex data pointer is null.");
      OTHER_ASSERT(index_data != nullptr, "Index data pointer is null.");
      OTHER_ASSERT(vertex_count != nullptr, "Vertex count pointer is null.");
      OTHER_ASSERT(index_count != nullptr, "Index count pointer is null.");

      driver* d = detail::get_dotnet_native_driver();
      OTHER_ASSERT(d != nullptr, "Driver is null in native_render_component_upload_mesh.");
      if (!d->rendering_enabled()) {
        CORE_LOG_WARN("Rendering is not enabled, cannot upload mesh data for object ID {}", object_id);
        return;
      }

      scene* active_scene = detail::get_active_scene_checked();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
      if (!active_scene->has_component<render_component>(object_id)) {
        CORE_LOG_ERROR("Object with ID {} does not have a render component, cannot upload mesh data.", object_id);
        return;
      }

      render_component* comp = active_scene->get_component<render_component>(object_id);
      OTHER_ASSERT(comp != nullptr, "Render component not found for object with ID {}", object_id);

      /// build mesh
      ostd::vector<vertex> vertices = detail::build_vertex_data(vertex_data, *vertex_count);
      ostd::vector<index> indices = detail::build_index_data(index_data, *index_count);

      std::string model_name = name;
      CORE_LOG_DEBUG("Uploading mesh data for object ID {} with name '{}', vertex count {}, index count {}", object_id, model_name, vertices.size(), indices.size());
      comp->model_asset_id = d->add_model_source_asset(model_name, vertices, indices);
      CORE_LOG_DEBUG(" - Uploaded mesh data for object ID {} with asset ID {} ({})", object_id, comp->model_asset_id, d->get_asset_hash(comp->model_asset_id));
    }

  }  // namespace bindings
  namespace detail {

    ostd::vector<vertex> build_vertex_data(const float* vertex_data, int32_t vertex_count) {
      ostd::vector<vertex> vertices(vertex_count);

      size_t cursor = 0;
      for (int32_t i = 0; i < vertex_count; ++i) {
        size_t start_cursor = cursor;

        vertex vtx;
        vtx.position = { vertex_data[cursor], vertex_data[cursor + 1], vertex_data[cursor + 2] };
        cursor += 3;

        vtx.normal = { vertex_data[cursor], vertex_data[cursor + 1], vertex_data[cursor + 2] };
        cursor += 3;

        vtx.tangent = { vertex_data[cursor], vertex_data[cursor + 1], vertex_data[cursor + 2] };
        cursor += 3;

        vtx.bitangent = { vertex_data[cursor], vertex_data[cursor + 1], vertex_data[cursor + 2] };
        cursor += 3;

        vtx.tex_coord = { vertex_data[cursor], vertex_data[cursor + 1] };
        cursor += 2;

        vtx.bone_ids = {
          static_cast<uint32_t>(vertex_data[cursor]),
          static_cast<uint32_t>(vertex_data[cursor + 1]),
          static_cast<uint32_t>(vertex_data[cursor + 2]),
          static_cast<uint32_t>(vertex_data[cursor + 3]),
        };
        cursor += 4;

        vtx.bone_weights = { vertex_data[cursor], vertex_data[cursor + 1], vertex_data[cursor + 2], vertex_data[cursor + 3] };
        cursor += 4;

        vertices[i] = vtx;
        OTHER_ASSERT(cursor - start_cursor == vertex::stride(), "Vertex data stride [{}] does not match expected vertex stride [{}]", cursor - start_cursor, vertex::stride());
      }
      return vertices;
    }

    ostd::vector<index> build_index_data(const int32_t* index_data, int32_t index_count) {
      ostd::vector<index> indices(index_count);

      size_t cursor = 0;
      for (int32_t i = 0; i < index_count; i++) {
        index idx;
        idx.v0 = static_cast<uint32_t>(index_data[cursor]);
        idx.v1 = static_cast<uint32_t>(index_data[cursor + 1]);
        idx.v2 = static_cast<uint32_t>(index_data[cursor + 2]);

        indices[i] = idx;
        cursor += 3;
      }
      return indices;
    }

  }  // namespace detail
}  // namespace other