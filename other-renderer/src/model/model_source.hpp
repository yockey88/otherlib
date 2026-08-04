/**
 * \file model/model_source.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP
#define OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP

#include "core/defines.hpp"
#include "core/ref_counted.hpp"

#include "gpu_resource/material.hpp"
#include "model.hpp"
#include "model/model_data.hpp"

namespace other {

  /// cpu-side model data plus the gpu handles for it; construction does no gpu work, so this is
  ///  buildable on any thread (renderer_backend::upload_model / destroy_model own the gpu half)
  class model_source : public ref_counted {
   public:
    explicit model_source(model_data&& data);
    ~model_source() override;  // asserts the gpu handles were already destroyed

    const model_data& source_data() const { return data; }

    inline std::string get_name() const { return data.name; }
    inline size_t get_num_vertices() const { return data.vertices.size(); }
    inline size_t get_num_indices() const { return data.indices.size(); }

    /// embedded-clip lookup; standalone .oanim clips live on the renderer_backend
    const animation_clip* find_clip(std::string_view name) const;

    model produce_model(const std::string& name = "", std::span<const uint32_t> submesh_idxs = {});

    /// value-set materials promoted from model_data::materials at load time, index-aligned
    ///  with submesh.material_index — derived data owned by the model, not assets or files;
    ///  a suzanne drops into a scene textured with zero authored .omat files
    const ostd::vector<material>& imported_materials() const { return imported; }
    ostd::vector<material>& imported_materials() { return imported; }
    void set_imported_materials(ostd::vector<material> materials) { imported = std::move(materials); }

    inline resource_handle get_mesh_handle() const { return mesh_handle; }
    inline bool uploaded() const { return mesh_handle.id != 0; }

    /// gpu state, written by renderer_backend::upload_model / destroy_model
    resource_handle mesh_handle = {};
    resource_handle vertex_buffer_handle = {};
    resource_handle index_buffer_handle = {};

   private:
    model_data data;
    ostd::vector<material> imported;

    size_t num_models_produced = 0;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP
