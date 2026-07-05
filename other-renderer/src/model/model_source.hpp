/**
 * \file model/model_source.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP
#define OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP

#include "core/defines.hpp"

#include "model.hpp"

namespace other {

  class model_source : public ref_counted {
   public:
    // clang-format off
   model_source(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices, const std::span<const triangle> triangle_map,
                const std::span<const submesh> submeshes, const std::span<const mesh_node> nodes, const std::span<const material> materials, const std::span<const animation> animations, 
                const skeleton& skeleton, const glm::mat4& global_transform, const glm::mat4& inverse_global_transform, const bounding_box& bounds);
    // clang-format on
    ~model_source();

    void destroy_resources();

    inline std::string get_name() const { return name; }

    model produce_model(const std::string& name = "", const std::span<const uint32_t> submesh_idxs = {});

    resource_handle get_mesh_handle() const;
    size_t get_num_vertices() const;
    size_t get_num_indices() const;

    /// non-const overloads (not all are provided)
    inline ostd::vector<vertex>& get_vertices() { return vertices; }
    inline ostd::vector<index>& get_indices() { return indices; }
    inline ostd::vector<triangle>& get_triangles() { return triangles; }
    inline ostd::vector<submesh>& get_submeshes() { return submeshes; }
    inline ostd::vector<mesh_node>& get_nodes() { return nodes; }
    inline bounding_box& get_bounding_box() { return bounds; }
    /// const overloads
    inline const glm::mat4& get_global_transform() const { return global_transform; }
    inline const glm::mat4& get_inverse_global_transform() const { return inverse_global_transform; }
    inline const ostd::vector<vertex>& get_vertices() const { return vertices; }
    inline const ostd::vector<index>& get_indices() const { return indices; }
    inline const ostd::vector<triangle>& get_triangles() const { return triangles; }
    inline const ostd::vector<submesh>& get_submeshes() const { return submeshes; }
    inline const ostd::vector<mesh_node>& get_nodes() const { return nodes; }
    inline const ostd::vector<material>& get_materials() const { return materials; }
    inline const ostd::vector<animation>& get_animations() const { return animations; }
    inline const bounding_box& get_bounding_box() const { return bounds; }

    animation* get_animation_by_name(const std::string& name);
    animation* get_animation(size_t index);

   private:
    friend struct model;

    std::string name;

    size_t num_models_produced = 0;

    skeleton* skel = nullptr;

    glm::mat4 global_transform = glm::mat4(1.0f);
    glm::mat4 inverse_global_transform = glm::mat4(1.0f);

    buffer_layout layout;

    /// \todo this is going to have to move to something with stable addressing consider how many pointers exist to
    ///        the data in these vectors

    /// base mesh data and geometry
    ostd::vector<vertex> vertices;
    ostd::vector<index> indices;
    ostd::vector<triangle> triangles;

    /// model structure data and transform relations for organizating geometry
    ostd::vector<submesh> submeshes;
    ostd::vector<mesh_node> nodes;

    /// high level model data
    ostd::vector<material> materials;
    ostd::vector<animation> animations;

    resource_handle mesh_handle;
    resource_handle vertex_buffer_handle;
    resource_handle index_buffer_handle;

    bounding_box bounds = bounding_box::empty;
    std::string file_path;

    ostd::vector<float> raw_vertices;
    ostd::vector<uint32_t> raw_indices;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_MODEL_SOURCE_HPP