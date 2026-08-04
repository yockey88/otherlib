/**
 * \file scripting/dotnet_bindings/component_bindings.cpp
 **/
#include "scripting/dotnet_bindings/component_bindings.hpp"

#include "thread/thread_safety.hpp"

#include "object/animation_component.hpp"
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