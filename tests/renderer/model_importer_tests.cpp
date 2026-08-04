/**
 * \file tests/renderer/model_importer_tests.cpp
 *
 * contract under test: model_importer::import is pure cpu (no gpu, no jobs), returns
 *  clean error results for bad inputs, and the skipped-mesh remap table keeps node,
 *  submesh, and rigging indices consistent when assimp meshes are rejected.
 **/
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "model/model_importer.hpp"

#include "other_test.hpp"

namespace other {

  class model_importer_tests : public other_test {
   protected:
    static const mesh_node* find_node(const model_data& data, const std::string_view name) {
      auto it = std::ranges::find_if(data.nodes, [&](const mesh_node& n) { return n.name == name; });
      return it == data.nodes.end() ? nullptr : &*it;
    }

    static bool any_warning_contains(const model_import_result& result, const std::string_view needle) {
      return std::ranges::any_of(result.warnings, [&](const std::string& w) { return w.find(needle) != std::string::npos; });
    }
  };

  TEST_F(model_importer_tests, import_fbx) {
    model_import_result result = import(filepath{ "tests/resources/models/suzanne3.fbx" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    EXPECT_TRUE(data.valid());
    EXPECT_EQ(data.name, "suzanne3");
    EXPECT_EQ(data.submeshes.size(), 3u);
    EXPECT_FALSE(data.nodes.empty());
    EXPECT_FALSE(data.materials.empty());
    EXPECT_LT(data.bounds.min.x, data.bounds.max.x);
    EXPECT_LT(data.bounds.min.y, data.bounds.max.y);

    /// submesh geometry ranges tile the shared vertex/index arrays without gaps
    uint32_t vertex_total = 0;
    uint32_t index_total = 0;
    for (const submesh& sm : data.submeshes) {
      EXPECT_EQ(sm.base_vertex, vertex_total);
      EXPECT_EQ(sm.base_idx, index_total);
      vertex_total += sm.vert_cnt;
      index_total += sm.idx_cnt;
    }
    EXPECT_EQ(vertex_total, data.vertices.size());
    EXPECT_EQ(index_total, data.indices.size() * 3);
  }

  TEST_F(model_importer_tests, import_gltf_binary_with_skeleton) {
    model_import_result result = import(filepath{ "tests/resources/models/bone-test-2-1.glb" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    EXPECT_TRUE(data.valid());
    ASSERT_EQ(data.submeshes.size(), 1u);
    EXPECT_TRUE(data.submeshes[0].rigged);
    EXPECT_EQ(data.skel.joints.size(), 2u);

    /// the rigging pass landed weights on the vertices through the remap table
    bool any_weighted = std::ranges::any_of(data.vertices, [](const vertex& v) {
      return v.bone_weights.x + v.bone_weights.y + v.bone_weights.z + v.bone_weights.w > 0.f;
    });
    EXPECT_TRUE(any_weighted);
  }

  /// regression anchor for the remap table: a mesh assimp cannot give
  ///  normals (points-only) sits BETWEEN two good meshes; before the table, every consumer
  ///  indexing submeshes by assimp mesh index desynced
  TEST_F(model_importer_tests, skipped_mesh_remap) {
    model_import_result result = import(filepath{ "tests/resources/models/skipped-mesh-remap.gltf" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    EXPECT_TRUE(any_warning_contains(result, "just_points"));
    ASSERT_EQ(data.submeshes.size(), 2u);
    EXPECT_EQ(data.submeshes[0].name, "tri_a");
    EXPECT_EQ(data.submeshes[1].name, "tri_b");
    EXPECT_EQ(data.vertices.size(), 6u);

    const mesh_node* good_a = find_node(data, "good_a");
    const mesh_node* good_b = find_node(data, "good_b");
    const mesh_node* degenerate = find_node(data, "degenerate");
    ASSERT_NE(good_a, nullptr);
    ASSERT_NE(good_b, nullptr);
    ASSERT_NE(degenerate, nullptr);

    ASSERT_EQ(good_a->sub_meshes.size(), 1u);
    ASSERT_EQ(good_b->sub_meshes.size(), 1u);
    EXPECT_TRUE(degenerate->sub_meshes.empty());
    EXPECT_EQ(data.submeshes[good_a->sub_meshes[0]].name, "tri_a");
    EXPECT_EQ(data.submeshes[good_b->sub_meshes[0]].name, "tri_b");
  }

  TEST_F(model_importer_tests, submesh_material_indices) {
    model_import_result result = import(filepath{ "tests/resources/models/two-materials.gltf" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    ASSERT_EQ(data.submeshes.size(), 2u);
    EXPECT_EQ(data.submeshes[0].material_index, 0u);
    EXPECT_EQ(data.submeshes[1].material_index, 1u);
    /// assimp appends its own default material, so >= the two authored ones
    ASSERT_GE(data.materials.size(), 2u);
    EXPECT_LT(data.submeshes[0].material_index, data.materials.size());
    EXPECT_LT(data.submeshes[1].material_index, data.materials.size());

    EXPECT_EQ(data.materials[0].name, "mat_red");
    EXPECT_EQ(data.materials[0].base_color, glm::vec4(1.f, 0.f, 0.f, 1.f));
    EXPECT_FLOAT_EQ(data.materials[0].roughness, 0.5f);
    EXPECT_FLOAT_EQ(data.materials[0].metalness, 0.25f);
    EXPECT_EQ(data.materials[1].name, "mat_blue");
    EXPECT_EQ(data.materials[1].base_color, glm::vec4(0.f, 0.f, 1.f, 1.f));
  }

  TEST_F(model_importer_tests, imported_material_textures) {
    model_import_result result = import(filepath{ "tests/resources/models/textured.gltf" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    ASSERT_FALSE(data.materials.empty());
    /// the uri is captured verbatim; resolving it is the material system's job
    EXPECT_EQ(data.materials[0].name, "textured_mat");
    EXPECT_EQ(data.materials[0].base_color_texture, "../textures/checker4x4.png");
  }

  TEST_F(model_importer_tests, missing_file_errors) {
    model_import_result result = import(filepath{ "tests/resources/models/does-not-exist.fbx" });
    EXPECT_FALSE(result.data.has_value());
    EXPECT_NE(result.error.find("does not exist"), std::string::npos) << result.error;
  }

  TEST_F(model_importer_tests, unsupported_extension_errors) {
    const filepath dir = std::filesystem::temp_directory_path() / "other-model-importer-tests";
    std::filesystem::create_directories(dir);
    const filepath bogus = dir / "not-a-model.xyz";
    {
      std::ofstream out(bogus);
      out << "payload";
    }

    model_import_result result = import(bogus);
    EXPECT_FALSE(result.data.has_value());
    EXPECT_NE(result.error.find("unsupported model format"), std::string::npos) << result.error;

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  TEST_F(model_importer_tests, omdl_reports_unimplemented) {
    const filepath dir = std::filesystem::temp_directory_path() / "other-model-importer-tests";
    std::filesystem::create_directories(dir);
    const filepath baked = dir / "ship.omdl";
    {
      std::ofstream out(baked);
      out << "OMDL";
    }

    model_import_result result = import(baked);
    EXPECT_FALSE(result.data.has_value());
    EXPECT_NE(result.error.find("not implemented"), std::string::npos) << result.error;

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

}  // namespace other
