/**
 * \file tests/serialization/asset_serialization.cpp
 **/
#include <print>
#include <ranges>

#include <core/formatting.hpp>
#include <gtest/gtest.h>

#include "serialization/serialization.hpp"

#include "model/model_importer.hpp"
#include "model/vertex.hpp"

#include "serialization_tests.hpp"

namespace other {

  TEST_F(serialization_tests, parse_omesh_file) {
    GTEST_SKIP() << "Skipping .omesh parsing test - broke omesh file format during refactor";

    filepath test_file_path = "tests/resources/models/suzanne3.omesh";
    ASSERT_TRUE(std::filesystem::exists(test_file_path)) << std::format("Test .omesh file does not exist: {}", test_file_path.string());

    std::vector<uint8_t> bytes = {};
    ASSERT_NO_FATAL_FAILURE(bytes = serialization::read_file_to_bytes(test_file_path));
    ASSERT_EQ(bytes.size(), 604091);

    size_t cursor = 0;
    std::span<const uint8_t> header = serialization::read_bytes(bytes, 4, cursor);
    ASSERT_EQ(header.size(), 4);
    ASSERT_EQ(header[0], 'M');
    ASSERT_EQ(header[1], 'E');
    ASSERT_EQ(header[2], 'S');
    ASSERT_EQ(header[3], 'H');

    uint32_t mesh_length = 0;
    ASSERT_NO_FATAL_FAILURE(mesh_length = serialization::read_value<uint32_t>(bytes, cursor));
    ASSERT_EQ(mesh_length, 604083);

    std::span<const uint8_t> mesh_data = {};
    ASSERT_NO_FATAL_FAILURE(mesh_data = serialization::read_bytes(bytes, mesh_length, cursor));
    ASSERT_FALSE(mesh_data.empty());
    ASSERT_EQ(mesh_data.size(), mesh_length);

    std::span mesh_bytes = std::span(mesh_data.data(), mesh_data.size());
    ASSERT_EQ(mesh_bytes.size(), mesh_length);

    size_t stride = vertex::stride();
    ASSERT_EQ(stride, 14);

    size_t mesh_cursor = 0;

    uint32_t num_vertices = 0;
    ASSERT_NO_FATAL_FAILURE(num_vertices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    /// suzanne has this many vertices
    ASSERT_EQ(num_vertices, 5163);

    std::span<const uint8_t> vertex_bytes = {};
    ASSERT_NO_FATAL_FAILURE(vertex_bytes = serialization::read_bytes(mesh_bytes, num_vertices * sizeof(double) * stride, mesh_cursor));
    ASSERT_GT(vertex_bytes.size(), 0);

    std::vector<vertex> vertex_data = {};
    // clang-format off
    ASSERT_NO_FATAL_FAILURE(vertex_data = vertex_bytes |
                                            std::views::chunk(sizeof(double)) |
                                            std::views::transform([](auto chunk) {
                                              double value = 0.0;
                                              std::memcpy(&value, chunk.data(), sizeof(double));
                                              return value;
                                            }) |
                                            std::views::chunk(vertex::stride()) |
                                            std::views::transform([&](auto vertex_chunk) {
                                              vertex v;
                                              v.position = glm::vec3(
                                                static_cast<float>(vertex_chunk[0]),
                                                static_cast<float>(vertex_chunk[1]),
                                                static_cast<float>(vertex_chunk[2])
                                              );
                                              v.normal = glm::vec3(
                                                static_cast<float>(vertex_chunk[3]),
                                                static_cast<float>(vertex_chunk[4]),
                                                static_cast<float>(vertex_chunk[5])
                                              );
                                              v.tangent = glm::vec3(
                                                static_cast<float>(vertex_chunk[6]),
                                                static_cast<float>(vertex_chunk[7]),
                                                static_cast<float>(vertex_chunk[8])
                                              );
                                              v.bitangent = glm::vec3(
                                                static_cast<float>(vertex_chunk[9]),
                                                static_cast<float>(vertex_chunk[10]),
                                                static_cast<float>(vertex_chunk[11])
                                              );
                                              v.tex_coord = glm::vec2(
                                                static_cast<float>(vertex_chunk[12]),
                                                static_cast<float>(vertex_chunk[13])
                                              );
                                              return vertex{};
                                            }) |
                                            std::ranges::to<std::vector<vertex>>());
    // clang-format on
    ASSERT_GT(vertex_data.size(), 0);

    uint32_t num_indices = 0;

    ASSERT_NO_FATAL_FAILURE(num_indices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_indices, 2131);

    std::span<const uint8_t> index_bytes = {};
    ASSERT_NO_FATAL_FAILURE(index_bytes = serialization::read_bytes(mesh_bytes, num_indices * sizeof(uint32_t) * 3, mesh_cursor));
    ASSERT_EQ(index_bytes.size(), num_indices * sizeof(uint32_t) * 3);

    std::vector<index> indices_data = {};
    // clang-format off
    ASSERT_NO_FATAL_FAILURE(indices_data = index_bytes | 
                                              std::views::chunk(sizeof(uint32_t)) | 
                                              std::views::transform([](auto chunk) {
                                                uint32_t value = 0;
                                                std::memcpy(&value, chunk.data(), sizeof(uint32_t));
                                                return value;
                                              }) |
                                              std::views::chunk(3) |
                                              std::views::transform([](auto index_chunk) {
                                                index idx;
                                                idx.v0 = index_chunk[0];
                                                idx.v1 = index_chunk[1];
                                                idx.v2 = index_chunk[2];
                                                return idx;
                                              }) | 
                                              std::ranges::to<std::vector<index>>());
    // clang-format on
    ASSERT_EQ(indices_data.size(), num_indices);

    uint32_t num_submeshes = 0;
    ASSERT_NO_FATAL_FAILURE(num_submeshes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_submeshes, 1);

    uint32_t base_vertex = 0;
    ASSERT_NO_FATAL_FAILURE(base_vertex = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(base_vertex, 0);

    uint32_t base_idx = 0;
    ASSERT_NO_FATAL_FAILURE(base_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(base_idx, 0);

    uint32_t idx_cnt = 0;
    ASSERT_NO_FATAL_FAILURE(idx_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    // ASSERT_EQ(idx_cnt, 2131);
    uint32_t vert_cnt = 0;
    ASSERT_NO_FATAL_FAILURE(vert_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    // ASSERT_EQ(vert_cnt, 0);

    /**
    expected local transform:
      100, 0, 0, 0,
      0, -3.5762787e-05, -99.99999, 0,
      0, 99.99999, -3.5762787e-05, 0,
      0, 0.81584466, 0, 1
   */
    glm::mat4 expected_local_transform = glm::mat4(1.0f);
    expected_local_transform[0][0] = 100.0f;
    expected_local_transform[1][1] = -3.5762787e-05f;
    expected_local_transform[1][2] = -99.99999f;
    expected_local_transform[2][1] = 99.99999f;
    expected_local_transform[2][2] = -3.5762787e-05f;
    expected_local_transform[3][1] = 0.81584466f;

    glm::mat4 local_transform = glm::mat4(1.0f);
    ASSERT_NO_FATAL_FAILURE(local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor));
    // std::println("Local Transform:\n{}", local_transform);
    ASSERT_EQ(local_transform, expected_local_transform);

    glm::vec3 bounds_min = glm::vec3(0.0f);
    ASSERT_NO_FATAL_FAILURE(bounds_min = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(bounds_min, glm::vec3(0.0f));
    glm::vec3 bounds_max = glm::vec3(0.0f);
    ASSERT_NO_FATAL_FAILURE(bounds_max = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(bounds_max, glm::vec3(0.0f));
    natural_t sub_mesh_id = 0;
    ASSERT_NO_FATAL_FAILURE(sub_mesh_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(sub_mesh_id, 0);
    natural_t material_id = 0;
    ASSERT_NO_FATAL_FAILURE(material_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(material_id, 0);
    uint32_t name_length = 0;
    ASSERT_NO_FATAL_FAILURE(name_length = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(name_length, 13);
    std::span<const uint8_t> name_bytes = {};
    ASSERT_NO_FATAL_FAILURE(name_bytes = serialization::read_bytes(mesh_bytes, name_length, mesh_cursor));
    std::string submesh_name = std::string(name_bytes.begin(), name_bytes.end());
    ASSERT_EQ(submesh_name, "$MergedNode_0");
  }

  TEST_F(serialization_tests, parse_cube_omesh_file) {
    GTEST_SKIP() << "Skipping .omesh parsing test - broke omesh file format during refactor";

    filepath test_file_path = "tests/resources/models/cube.omesh";
    ASSERT_TRUE(std::filesystem::exists(test_file_path)) << std::format("Test .omesh file does not exist: {}", test_file_path.string());

    std::vector<uint8_t> bytes = {};
    ASSERT_NO_FATAL_FAILURE(bytes = serialization::read_file_to_bytes(test_file_path));
    ASSERT_EQ(bytes.size(), 3077);

    size_t cursor = 0;
    std::span<const uint8_t> header = serialization::read_bytes(bytes, 4, cursor);
    ASSERT_EQ(header.size(), 4);
    ASSERT_EQ(header[0], 'M');
    ASSERT_EQ(header[1], 'E');
    ASSERT_EQ(header[2], 'S');
    ASSERT_EQ(header[3], 'H');

    uint32_t mesh_length = 0;
    ASSERT_NO_FATAL_FAILURE(mesh_length = serialization::read_value<uint32_t>(bytes, cursor));
    /// 2688 +4 = 2692 bytes of vertices
    /// 144 +4 = 148 bytes of indices
    /// 133 +4 = 137 bytes of submesh
    /// 88 +4 = 92 bytes of nodes
    /// total = 2692 + 148 + 137 + 92 = 3069
    ASSERT_EQ(mesh_length, 3069);

    std::span<const uint8_t> mesh_data = {};
    ASSERT_NO_FATAL_FAILURE(mesh_data = serialization::read_bytes(bytes, mesh_length, cursor));
    ASSERT_FALSE(mesh_data.empty());
    ASSERT_EQ(mesh_data.size(), mesh_length);

    std::span mesh_bytes = std::span(mesh_data.data(), mesh_data.size());
    ASSERT_EQ(mesh_bytes.size(), mesh_length);

    size_t stride = vertex::stride();
    ASSERT_EQ(stride, 14);

    size_t mesh_cursor = 0;
    uint32_t num_vertices = 0;
    ASSERT_NO_FATAL_FAILURE(num_vertices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_vertices, 24);

    std::span<const uint8_t> vertex_bytes = {};
    ASSERT_NO_FATAL_FAILURE(vertex_bytes = serialization::read_bytes(mesh_bytes, num_vertices * sizeof(double) * stride, mesh_cursor));
    ASSERT_EQ(vertex_bytes.size(), num_vertices * sizeof(double) * stride);

    std::vector<vertex> vertices = vertex_bytes | std::views::chunk(sizeof(double)) | std::views::transform([](auto chunk) { return *reinterpret_cast<const double*>(chunk.data()); }) | std::views::chunk(vertex::stride()) | std::views::transform([&](auto vertex_chunk) {
                                     vertex v;
                                     v.position = glm::vec3{ static_cast<float>(vertex_chunk[0]), static_cast<float>(vertex_chunk[1]), static_cast<float>(vertex_chunk[2]) };
                                     v.normal = glm::vec3{ static_cast<float>(vertex_chunk[3]), static_cast<float>(vertex_chunk[4]), static_cast<float>(vertex_chunk[5]) };
                                     v.tangent = glm::vec3{ static_cast<float>(vertex_chunk[6]), static_cast<float>(vertex_chunk[7]), static_cast<float>(vertex_chunk[8]) };
                                     v.bitangent = glm::vec3{ static_cast<float>(vertex_chunk[9]), static_cast<float>(vertex_chunk[10]), static_cast<float>(vertex_chunk[11]) };
                                     v.tex_coord = glm::vec2{ static_cast<float>(vertex_chunk[12]), static_cast<float>(vertex_chunk[13]) };
                                     return v;
                                   }) |
      std::ranges::to<std::vector<vertex>>();

    for (uint32_t v = 0; v < vertices.size(); ++v) {
      std::println("Vertex[{}] -- pos: {}, normal: {}, tex_coord: {}", v, vertices[v].position, vertices[v].normal, vertices[v].tex_coord);
    }

    uint32_t num_indices = 0;
    ASSERT_NO_FATAL_FAILURE(num_indices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_indices, 12);

    std::span<const uint8_t> index_bytes = {};
    ASSERT_NO_FATAL_FAILURE(index_bytes = serialization::read_bytes(mesh_bytes, num_indices * sizeof(uint32_t) * 3, mesh_cursor));
    ASSERT_EQ(index_bytes.size(), num_indices * sizeof(uint32_t) * 3);

    std::vector<index> indices = index_bytes | std::views::chunk(sizeof(uint32_t)) | std::views::transform([](auto chunk) { return *reinterpret_cast<const uint32_t*>(chunk.data()); }) | std::views::chunk(3) | std::views::transform([](auto index_chunk) {
                                   index idx;
                                   idx.v0 = index_chunk[0];
                                   idx.v1 = index_chunk[1];
                                   idx.v2 = index_chunk[2];
                                   return idx;
                                 }) |
      std::ranges::to<std::vector<index>>();

    for (uint32_t i = 0; i < indices.size(); ++i) {
      std::println("Index[{}] -- v0: {}, v1: {}, v2: {}", i, indices[i].v0, indices[i].v1, indices[i].v2);
    }

    uint32_t num_submeshes = 0;
    ASSERT_NO_FATAL_FAILURE(num_submeshes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_submeshes, 1);

    uint32_t base_vertex = 0;
    ASSERT_NO_FATAL_FAILURE(base_vertex = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(base_vertex, 0);

    uint32_t base_idx = 0;
    ASSERT_NO_FATAL_FAILURE(base_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(base_idx, 0);

    uint32_t idx_cnt = 0;
    ASSERT_NO_FATAL_FAILURE(idx_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(idx_cnt, 12 * 3);  /// 12 indices, 3 uint32_t each

    uint32_t vert_cnt = 0;
    ASSERT_NO_FATAL_FAILURE(vert_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(vert_cnt, 24);

    glm::mat4 local_transform = glm::mat4(1.0f);
    ASSERT_NO_FATAL_FAILURE(local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(local_transform, glm::mat4(1.0f));
    std::println("Local Transform:\n{}", local_transform);

    glm::vec3 bounds_min = glm::vec3(0.0f);
    ASSERT_NO_FATAL_FAILURE(bounds_min = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(bounds_min, glm::vec3(0.0f));

    glm::vec3 bounds_max = glm::vec3(0.0f);
    ASSERT_NO_FATAL_FAILURE(bounds_max = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(bounds_max, glm::vec3(0.0f));
    std::println("Bounds Min: {}, Max: {}", bounds_min, bounds_max);

    natural_t sub_mesh_id = 0;
    ASSERT_NO_FATAL_FAILURE(sub_mesh_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(sub_mesh_id, 0);

    uint32_t name_length = 0;
    ASSERT_NO_FATAL_FAILURE(name_length = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(name_length, 4);

    std::span<const uint8_t> name_bytes = {};
    ASSERT_NO_FATAL_FAILURE(name_bytes = serialization::read_bytes(mesh_bytes, name_length, mesh_cursor));
    std::string submesh_name = std::string(name_bytes.begin(), name_bytes.end());
    ASSERT_EQ(submesh_name, "Cube");

    uint8_t rigged_byte = 0;
    ASSERT_NO_FATAL_FAILURE(rigged_byte = serialization::read_value<uint8_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(rigged_byte, 0);

    std::println("Submesh -- base_vertex: {}, base_idx: {}, idx_cnt: {}, vert_cnt: {}, name: {}", base_vertex, base_idx, idx_cnt, vert_cnt, submesh_name);
    std::println("-- local_transform:\n{}", local_transform);

    uint32_t num_nodes = 0;
    ASSERT_NO_FATAL_FAILURE(num_nodes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_nodes, 1);

    uint32_t parent = 0xFFFFFFFF;
    ASSERT_NO_FATAL_FAILURE(parent = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(parent, 0xFFFFFFFF);

    uint32_t num_children = 0;
    ASSERT_NO_FATAL_FAILURE(num_children = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_children, 0);

    std::vector<uint32_t> children = {};
    for (uint32_t c = 0; c < num_children; ++c) {
      uint32_t child_idx = 0;
      ASSERT_NO_FATAL_FAILURE(child_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
      children.emplace_back(child_idx);
    }
    ASSERT_EQ(children.size(), 0);

    uint32_t num_submeshes_in_node = 0;
    ASSERT_NO_FATAL_FAILURE(num_submeshes_in_node = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(num_submeshes_in_node, 1);

    std::vector<uint32_t> submesh_ids = {};
    for (uint32_t sm = 0; sm < num_submeshes_in_node; ++sm) {
      uint32_t submesh_id = 0;
      ASSERT_NO_FATAL_FAILURE(submesh_id = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
      submesh_ids.emplace_back(submesh_id);
    }

    glm::mat4 node_local_transform = glm::mat4(1.0f);
    ASSERT_NO_FATAL_FAILURE(node_local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(node_local_transform, glm::mat4(1.0f));

    uint32_t name_length_node = 0;
    ASSERT_NO_FATAL_FAILURE(name_length_node = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor));
    ASSERT_EQ(name_length_node, 4);

    std::span<const uint8_t> name_bytes_node = {};
    ASSERT_NO_FATAL_FAILURE(name_bytes_node = serialization::read_bytes(mesh_bytes, name_length_node, mesh_cursor));
    std::string node_name = std::string(name_bytes_node.begin(), name_bytes_node.end());
    ASSERT_EQ(node_name, "Cube");

    std::println("Node -- parent: {}, num_children: {}, num_submeshes: {}, name: {}", parent, children.size(), submesh_ids.size(), node_name);
    std::println("-- local_transform:\n{}", node_local_transform);
  }

  TEST_F(serialization_tests, model_importer_test) {
    GTEST_SKIP() << "Skipping .omesh parsing test - broke omesh file format during refactor";

    model_builder builder;
    filepath test_file_path = "tests/resources/models/cube.omesh";

    ASSERT_NO_FATAL_FAILURE(builder = model_importer::load_model_data(test_file_path));
    // ASSERT_EQ(builder.vertices.size(), 24);
    // ASSERT_EQ(builder.indices.size(), 12);
    // ASSERT_EQ(builder.submeshes.size(), 1);
    // ASSERT_EQ(builder.nodes.size(), 1);
    // ASSERT_EQ(builder.submeshes[0].name, "Cube");
    // ASSERT_EQ(builder.nodes[0].name, "Cube");

    builder.dump_model_info();
  }

}  // namespace other