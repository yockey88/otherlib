/**
 * \file tests/serialization/asset_serialization.cpp
 **/
#include <ranges>

#include <gtest/gtest.h>

#include "serialization/serialization.hpp"

#include "model/vertex.hpp"

#include "serialization_tests.hpp"

namespace other {

  TEST_F(serialization_tests, parse_omesh_file) {
    filepath test_file_path = "tests/resources/models/suzanne3.omesh";
    ASSERT_TRUE(std::filesystem::exists(test_file_path)) << std::format("Test .omesh file does not exist: {}", test_file_path.string());

    std::vector<uint8_t> bytes = {};
    ASSERT_NO_FATAL_FAILURE(bytes = serialization::read_file_to_bytes(test_file_path));
    /// this is how many bytes suzanne is made of :D
    ASSERT_EQ(bytes.size(), 604091);

    size_t cursor = 0;
    std::vector<uint8_t> header = serialization::read_bytes(bytes, 4, cursor);
    ASSERT_EQ(header.size(), 4);
    ASSERT_EQ(header[0], 'M');
    ASSERT_EQ(header[1], 'E');
    ASSERT_EQ(header[2], 'S');
    ASSERT_EQ(header[3], 'H');

    uint32_t mesh_length = 0;
    ASSERT_NO_FATAL_FAILURE(mesh_length = serialization::read_value<uint32_t>(bytes, cursor));
    ASSERT_GT(mesh_length, 0);

    size_t start_of_vertex_index_data = cursor;
    size_t vertex_index_cursor = 0;

    std::vector<uint8_t> mesh_data = {};
    ASSERT_NO_FATAL_FAILURE(mesh_data = serialization::read_bytes(bytes, mesh_length, cursor));
    ASSERT_FALSE(mesh_data.empty());

    std::span mesh_bytes = mesh_data;

    size_t stride = vertex::stride();
    ASSERT_EQ(stride, 14);

    uint32_t num_vertices = 0;
    ASSERT_NO_FATAL_FAILURE(num_vertices = serialization::read_value<uint32_t>(mesh_bytes, vertex_index_cursor));
    /// suzanne has this many vertices
    ASSERT_EQ(num_vertices, 5163);
    ASSERT_GT(mesh_bytes.size(), num_vertices * sizeof(double) * stride);

    std::vector<uint8_t> vertex_bytes = {};
    ASSERT_NO_FATAL_FAILURE(vertex_bytes = serialization::read_bytes(mesh_bytes.subspan(sizeof(uint32_t)), num_vertices * sizeof(double) * stride, vertex_index_cursor));
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
                                            std::ranges::to<std::vector>() |
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
    ASSERT_NO_FATAL_FAILURE(num_indices = serialization::read_value<uint32_t>(mesh_bytes.subspan(vertex_index_cursor), vertex_index_cursor));
    ASSERT_EQ(num_indices, 2131);
    ASSERT_GT(mesh_bytes.size(), sizeof(uint32_t) + (num_vertices * sizeof(double) * stride) + sizeof(uint32_t) + (num_indices + sizeof(uint32_t) * 3));

    std::vector<uint8_t> indices_bytes = {};
    ASSERT_NO_FATAL_FAILURE(vertex_bytes = serialization::read_bytes(mesh_bytes.subspan(vertex_index_cursor), num_indices + sizeof(uint32_t) * 3, vertex_index_cursor));
    ASSERT_EQ(indices_bytes.size(), num_indices + sizeof(uint32_t) * 3);

    std::vector<index> indices_data = {};
    // clang-format off
    ASSERT_NO_FATAL_FAILURE(indices_data = indices_bytes | 
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
    // ASSERT_GT(indices_data.size(), 0);

    // uint32_t mesh_length = serialization::read_value<uint32_t>(bytes, cursor);
    // if (mesh_length == 0) {
    //   on_failure(std::format("Invalid mesh length in file: {}", file_path.string()));
    //   return;
    // }

    // size_t start_of_vertex_index_data = cursor;
    // size_t vertex_index_cursor = 0;
    // std::vector<uint8_t> vertex_index_data = serialization::read_bytes(bytes, mesh_length, cursor);
    // if (vertex_index_data.empty()) {
    //   on_failure(std::format("Failed to read vertex index data in file: {}", file_path.string()));
    //   return;
    // }
  }

}  // namespace other