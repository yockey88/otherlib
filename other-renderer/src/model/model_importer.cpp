/**
 * \file model/model_importer.cpp
 **/
#include "model/model_importer.hpp"

#include <format>
#include <string>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <mio/mmap.hpp>

#include "core/formatting.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "serialization/serialization.hpp"

#include "assimp/matrix4x4.h"

namespace other {

  void model_builder::dump_model_info() const {
    std::stringstream ss;
    ss << std::format("Model Info [{}]:", submeshes[0].name);
    ss << std::format("\n - Vertex Count: {}", vertices.size());
    for (uint32_t v = 0; v < vertices.size(); ++v) {
      const vertex& vert = vertices[v];
      ss << std::format("\n   - Vertex[{}] -- pos: {}, normal: {}, tangent: {}, bitangent: {}, tex_coord: {}", v, vert.position, vert.normal, vert.tangent, vert.bitangent, vert.tex_coord);
    }

    ss << std::format("\n - Index Count: {}", indices.size());
    for (uint32_t i = 0; i < indices.size(); ++i) {
      const index& idx = indices[i];
      ss << std::format("\n   - Index[{}] -- v0: {}, v1: {}, v2: {}", i, idx.v0, idx.v1, idx.v2);
    }

    ss << std::format("\n - Submesh Count: {}", submeshes.size());
    for (uint32_t s = 0; s < submeshes.size(); ++s) {
      const submesh& submesh = submeshes[s];
      ss << std::format("\n   - Submesh[{}] -- base_vertex: {}, base_idx: {}, mat_idx: {}, idx_cnt: {}, vert_cnt: {}, name: {}", s, submesh.base_vertex, submesh.base_idx, submesh.mat_idx, submesh.idx_cnt, submesh.vert_cnt, submesh.name);
      ss << std::format("\n   -- local_transform:\n{}", submesh.local_transform);
    }

    ss << std::format("\n - Node Count: {}", nodes.size());
    ss << std::format("\n - Bounds: [min: {}, max: {}]", bounds.min, bounds.max);
    for (uint32_t n = 0; n < nodes.size(); ++n) {
      const mesh_node& node = nodes[n];
      ss << std::format("\n   - Node[{}] -- parent: {}, num_children: {}, num_submeshes: {}, name: {}", n, node.parent, node.children.size(), node.sub_meshes.size(), node.name);
      ss << std::format("\n   -- local_transform:\n{}", node.local_transform);
    }
    CORE_LOG_DEBUG("{}", ss.str());
  }

  namespace model_importer {
    namespace detail {

      void process_other_mesh_file(const filepath& file_path, model_builder& builder);
      void process_fbx_file(const filepath& file_path, model_builder& builder);

      void process_assimp_meshes(const aiScene* scene, model_builder& builder);
      void traverse_assimp_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform = glm::mat4(1.f), uint32_t level = 0);
      void process_assimp_materials(size_t idx, const aiScene* scene, model_builder& builder);

    }  // namespace detail

    model_builder load_model_data(const filepath& file_path) {
      PROFILE_SECTION("model_importer::load_model_data");

      model_builder builder;
      OTHER_ASSERT(std::filesystem::exists(file_path), "Model file does not exist: {}", file_path.string());

      std::string extension = file_path.extension().string();
      if (extension == ".omesh") {
        detail::process_other_mesh_file(file_path, builder);
      } else if (extension == ".fbx" || extension == ".obj") {
        detail::process_fbx_file(file_path, builder);
      } else {
        CORE_LOG_ERROR("Unsupported model file extension: {}", extension);
      }

      return builder;
    }

    model_builder build_model_data(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
      PROFILE_SECTION("model_importer::build_model_data--from-vertices");

      model_builder builder;

      builder.vertices = vertices;
      builder.indices = indices;

      if (vertices.empty() || indices.empty()) {
        CORE_LOG_ERROR("Model data is empty");
        return builder;
      }

      submesh& submesh = builder.submeshes.emplace_back();
      submesh.base_vertex = 0;
      submesh.base_idx = 0;
      submesh.mat_idx = 0;
      submesh.vert_cnt = static_cast<uint32_t>(vertices.size());
      submesh.idx_cnt = static_cast<uint32_t>(indices.size() * 3);
      submesh.sub_mesh_id = 0;
      submesh.material_id = 0;
      submesh.local_transform = glm::mat4(1.0f);
      submesh.name = name;

      builder.bounds = bounding_box::empty;
      for (const auto& vertex : vertices) {
        builder.bounds.min.x = glm::min(builder.bounds.min.x, vertex.position.x);
        builder.bounds.min.y = glm::min(builder.bounds.min.y, vertex.position.y);
        builder.bounds.min.z = glm::min(builder.bounds.min.z, vertex.position.z);
        builder.bounds.max.x = glm::max(builder.bounds.max.x, vertex.position.x);
        builder.bounds.max.y = glm::max(builder.bounds.max.y, vertex.position.y);
        builder.bounds.max.z = glm::max(builder.bounds.max.z, vertex.position.z);
      }

      builder.triangles.reserve(indices.size() / 3);
      uint32_t i = 0;
      for (const auto& idx : indices) {
        triangle& tri = builder.triangles[i].emplace_back();
        tri.v0 = vertices[idx.v0];
        tri.v1 = vertices[idx.v1];
        tri.v2 = vertices[idx.v2];
        tri.centroid = (tri.v0.position + tri.v1.position + tri.v2.position) / 3.0f;
        i++;
      }

      mesh_node& _ = builder.nodes.emplace_back();
      return builder;
    }

    namespace detail {

      namespace {

        glm::mat4 mat4_from_ai_mat4(const aiMatrix4x4& matrix) {
          glm::mat4 result;
          // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
          result[0][0] = matrix.a1;
          result[1][0] = matrix.a2;
          result[2][0] = matrix.a3;
          result[3][0] = matrix.a4;
          result[0][1] = matrix.b1;
          result[1][1] = matrix.b2;
          result[2][1] = matrix.b3;
          result[3][1] = matrix.b4;
          result[0][2] = matrix.c1;
          result[1][2] = matrix.c2;
          result[2][2] = matrix.c3;
          result[3][2] = matrix.c4;
          result[0][3] = matrix.d1;
          result[1][3] = matrix.d2;
          result[2][3] = matrix.d3;
          result[3][3] = matrix.d4;
          return result;
        }

      }  // anonymous namespace

      void process_other_mesh_file(const filepath& file_path, model_builder& builder) {
        auto on_failure = [](const std::string& msg) {
          CORE_LOG_ERROR("model_importer::process_other_mesh_file -- {}", msg);
        };

        /// use memory-mapped io so this is as fast as possible
        std::error_code ec;
        mio::mmap_source file = mio::make_mmap_source(file_path.string(), ec);
        if (ec) {
          on_failure(std::format("Failed to mmap file: {}", file_path.string()));
          return;
        }

        std::span<const uint8_t> bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(file.data()), file.size());
        if (bytes.empty()) {
          on_failure(std::format("Failed to read file bytes: {}", file_path.string()));
          return;
        }

        /**
          | <mesh-block> | <mesh-length> | <vertex-index-data> | <submeshes> | <nodes> |
          |------------------------------|---------------------------------------------|
          | 4 bytes      | 4 bytes       |                     |             |         |

        vertex-index-data
          | <num-vertices> | <vertices> (stride of 7 doubles each) | <index-data> |
          |-----------------------------------------------------------------------|
          | 4 bytes        | 7 * <num-vertices> * 8 bytes          |              |

        index-data
          | <num-indices> | <indices> (stride of 3 uint32_t each) |
          |-------------------------------------------------------|
          | 4 bytes       | 3 * <num-indices> * 4 bytes           |

        submeshes
          | <num-submeshes> | <submesh-data> |
          |----------------------------------|
          | 4 bytes         |                |

        nodes
          | <num-nodes> | <node-data> |
          |---------------------------|
          | 4 bytes     |             |

          triangle-data
          | <num-triangles> | <triangle-data> |
          |-----------------------------------|
          | 4 bytes         |                 |

        submesh-data
          | <base-vertex> | <base-idx> | <mat-idx> | <idx-cnt> | <vert-cnt> | <local-transform> | <bounds> | <sub-mesh-id> | <material-id> | <name-length> | <name> | <rigged> |
          |--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
          | 4 bytes       | 4 bytes    | 4 bytes   | 4 bytes   | 4 bytes    | 64 bytes          | 24 bytes | 8 bytes       | 8 bytes       | 4 bytes       |        | 1 byte   |

        node-data
          | <parent> | <num-children> | <children>         | <num-submeshes> | <submesh-ids>       | <local-transform> | <name-length> | <name> |
          |-------------------------------------------------------------------------------------------------------------------------------------|
          | 4 bytes  | 4 bytes        | 4 * <num-children> | 4 bytes         | 4 * <num-submeshes> | 64 bytes          | 4 bytes       |        |
        **/

        size_t cursor = 0;
        std::span<const uint8_t> header = serialization::read_bytes(bytes, 4, cursor);
        if (header.size() != 4 || header[0] != 'M' || header[1] != 'E' || header[2] != 'S' || header[3] != 'H') {
          on_failure(std::format("Invalid mesh block in file: {}", file_path.string()));
          return;
        }

        uint32_t mesh_length = serialization::read_value<uint32_t>(bytes, cursor);
        if (mesh_length == 0) {
          on_failure(std::format("Invalid mesh length in file: {}", file_path.string()));
          return;
        }

        std::span<const uint8_t> mesh_data = serialization::read_bytes(bytes, mesh_length, cursor);
        std::span mesh_bytes = std::span(mesh_data.data(), mesh_data.size());
        if (mesh_bytes.empty()) {
          on_failure(std::format("Failed to read mesh bytes in file: {}", file_path.string()));
          return;
        }

        size_t stride = vertex::stride();
        size_t mesh_cursor = 0;

        uint32_t num_vertices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
        std::span<const uint8_t> vertex_bytes = serialization::read_bytes(mesh_bytes, num_vertices * sizeof(double) * stride, mesh_cursor);
        if (vertex_bytes.empty()) {
          on_failure(std::format("Failed to read vertex data in file: {}", file_path.string()));
          return;
        }
        OTHER_ASSERT(vertex_bytes.size() == num_vertices * sizeof(double) * stride, "Vertex data size mismatch in file: {}", file_path.string());

        // clang-format off
        builder.vertices = vertex_bytes
          | std::views::chunk(sizeof(double))
          | std::views::transform([](auto chunk) { return *reinterpret_cast<const double*>(chunk.data()); })
          | std::views::chunk(vertex::stride())
          | std::views::transform([&](auto vertex_chunk) {
                             vertex v;
                             v.position = glm::vec3(static_cast<float>(vertex_chunk[0]), static_cast<float>(vertex_chunk[1]), static_cast<float>(vertex_chunk[2]));
                             v.normal = glm::vec3(static_cast<float>(vertex_chunk[3]), static_cast<float>(vertex_chunk[4]), static_cast<float>(vertex_chunk[5]));
                             v.tangent = glm::vec3(static_cast<float>(vertex_chunk[6]), static_cast<float>(vertex_chunk[7]), static_cast<float>(vertex_chunk[8]));
                             v.bitangent = glm::vec3(static_cast<float>(vertex_chunk[9]), static_cast<float>(vertex_chunk[10]), static_cast<float>(vertex_chunk[11]));
                             v.tex_coord = glm::vec2(static_cast<float>(vertex_chunk[12]), static_cast<float>(vertex_chunk[13]));
                             return v;
                           })
          | std::ranges::to<std::vector<vertex>>();
        // clang-format on

        uint32_t num_indices = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
        std::span<const uint8_t> index_bytes = serialization::read_bytes(mesh_bytes, num_indices * sizeof(uint32_t) * 3, mesh_cursor);
        if (index_bytes.empty()) {
          on_failure(std::format("Failed to read index data in file: {}", file_path.string()));
          return;
        }
        OTHER_ASSERT(index_bytes.size() == num_indices * sizeof(uint32_t) * 3, "Index data size mismatch in file: {}", file_path.string());

        // clang-format off
        builder.indices = index_bytes
          | std::views::chunk(sizeof(uint32_t))
          | std::views::transform([](auto chunk) { return *reinterpret_cast<const uint32_t*>(chunk.data()); })
          | std::views::chunk(3)
          | std::views::transform([](auto index_chunk) {
                            index idx;
                            idx.v0 = index_chunk[0];
                            idx.v1 = index_chunk[1];
                            idx.v2 = index_chunk[2];
                            return idx;
                          })
          | std::ranges::to<std::vector<index>>();
        // clang-format on

        uint32_t num_submeshes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
        for (uint32_t s = 0; s < num_submeshes; ++s) {
          submesh submesh;
          submesh.base_vertex = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          submesh.base_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          submesh.mat_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          submesh.idx_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          submesh.vert_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);

          submesh.local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor);

          submesh.bounds.min = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor);
          submesh.bounds.max = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor);
          submesh.sub_mesh_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor);
          submesh.material_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor);

          uint32_t name_length = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          submesh.name = serialization::read_string_value(mesh_bytes, name_length, mesh_cursor);

          uint8_t rigged_byte = serialization::read_value<uint8_t>(mesh_bytes, mesh_cursor);
          submesh.rigged = rigged_byte != 0;

          builder.submeshes.emplace_back(submesh);
        }

        uint32_t num_nodes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
        for (uint32_t n = 0; n < num_nodes; ++n) {
          mesh_node node;

          node.parent = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);

          uint32_t num_children = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          for (uint32_t c = 0; c < num_children; ++c) {
            uint32_t child_idx = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
            node.children.emplace_back(child_idx);
          }

          uint32_t num_submeshes = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          for (uint32_t sm = 0; sm < num_submeshes; ++sm) {
            uint32_t submesh_id = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
            node.sub_meshes.emplace_back(submesh_id);
          }

          node.local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor);

          uint32_t name_length = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
          node.name = serialization::read_string_value(mesh_bytes, name_length, mesh_cursor);

          builder.nodes.emplace_back(node);
        }

        builder.triangles.reserve((builder.indices.size() / 3) + 1);
        size_t i = 0;
        for (auto& idx : builder.indices) {
          triangle& tri = builder.triangles[i].emplace_back();
          tri.v0 = builder.vertices[idx.v0];
          tri.v1 = builder.vertices[idx.v1];
          tri.v2 = builder.vertices[idx.v2];
          tri.centroid = (tri.v0.position + tri.v1.position + tri.v2.position) / 3.0f;

          ++i;
        }
      }

      void process_fbx_file(const filepath& file_path, model_builder& builder) {
        Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
        Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_STDOUT);

        std::string log_file = std::format("logs/{}-import-info.log", file_path.filename().stem().string());
        Assimp::DefaultLogger::create(log_file.c_str(), severity, aiDefaultLogStream_FILE);
        Assimp::DefaultLogger::get()->info("begin import of model: ", file_path.string());

        OTHER_ASSERT(std::filesystem::exists(file_path), "{} does not exist", file_path.string());

        uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate |
          // // aiProcess_SortByPType |
          // // aiProcess_GenNormals |
          aiProcess_GenUVCoords | aiProcess_OptimizeGraph |
          // // aiProcess_RemoveRedundantMaterials |
          aiProcess_FindDegenerates | aiProcess_FindInvalidData |
          // // aiProcess_TransformUVCoords |
          aiProcess_FindInstances |
          // // aiProcess_SplitByBoneCount |
          aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
          // // aiProcess_LimitBoneWeights |
          aiProcess_ValidateDataStructure |
          // e.g. convert cm to m for fbx import (and other formats where cm is native)
          aiProcess_GlobalScale;

        Assimp::Importer importer;
        const aiScene* scene = nullptr;
        {
          PROFILE_SECTION("model_importer::load_model_data--read-file");

          scene = importer.ReadFile(file_path.string(), flags);
          if (scene == nullptr) {
            CORE_LOG_ERROR("Failed to load model : {} [{}]", file_path.string(), importer.GetErrorString());
            return;
          }
        }

        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
          CORE_LOG_ERROR("Failed to load model : {}\n[ASSIMP ERROR : {}]", file_path.string(), importer.GetErrorString());
          return;
        }

        CORE_LOG_DEBUG("Model loaded successfully: {}", file_path.string());
        Assimp::DefaultLogger::get()->info("Model loaded successfully: ", file_path.string());

        // ProcessMaterials(scene);
        {
          PROFILE_SECTION("model_importer::load_model_data--process-meshes");
          detail::process_assimp_meshes(scene, builder);
        }

        {
          PROFILE_SECTION("model_importer::load_model_data--traverse-nodes");

          mesh_node& _ = builder.nodes.emplace_back();
          traverse_assimp_nodes(scene->mRootNode, 0, builder);
        }

        {
          PROFILE_SECTION("model_importer::load_model_data--compute-bounds");

          builder.bounds = bounding_box::empty;
          /// compute overall model bounds
          for (const auto& submesh : builder.submeshes) {
            bounding_box submesh_bounds = submesh.bounds;
            glm::vec3 min = glm::vec3(glm::mat4(1.0f) * glm::vec4(submesh_bounds.min, 1.0f));
            glm::vec3 max = glm::vec3(glm::mat4(1.0f) * glm::vec4(submesh_bounds.max, 1.0f));

            builder.bounds.min.x = glm::min(builder.bounds.min.x, min.x);
            builder.bounds.min.y = glm::min(builder.bounds.min.y, min.y);
            builder.bounds.min.z = glm::min(builder.bounds.min.z, min.z);
            builder.bounds.max.x = glm::max(builder.bounds.max.x, max.x);
            builder.bounds.max.y = glm::max(builder.bounds.max.y, max.y);
            builder.bounds.max.z = glm::max(builder.bounds.max.z, max.z);
          }
        }
      }

      void process_assimp_meshes(const aiScene* scene, model_builder& builder) {
        PROFILE_SECTION("model_importer::load_model_data--process-submeshes");

        uint32_t vertex_count = 0;
        uint32_t idx_count = 0;

        for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
          aiMesh* mesh = scene->mMeshes[i];
          OTHER_ASSERT(mesh != nullptr, "Failed to get mesh");

          submesh& submesh = builder.submeshes.emplace_back();
          submesh.sub_mesh_id = i;

          submesh.base_vertex = vertex_count;
          submesh.base_idx = idx_count;

          submesh.material_id = mesh->mMaterialIndex;
          submesh.vert_cnt = mesh->mNumVertices;
          submesh.idx_cnt = mesh->mNumFaces * 3;

          submesh.name = mesh->mName.C_Str();

          if (!mesh->HasPositions()) {
            CORE_LOG_WARN("Mesh index {} with name '{}' has no vertex positions - skipping import!", i, mesh->mName.C_Str());
          }
          if (!mesh->HasNormals()) {
            CORE_LOG_WARN("Mesh index {} with name '{}' has no vertex normals, and they could not be computed - skipping import!", i, mesh->mName.C_Str());
          }
          bool skip = !mesh->HasPositions() || !mesh->HasNormals();
          if (skip) {
            continue;
          }

          for (uint32_t j = 0; j < mesh->mNumVertices; ++j) {
            vertex& vertex = builder.vertices.emplace_back();
            vertex.position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
            vertex.normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };

            if (mesh->HasTangentsAndBitangents()) {
              vertex.tangent = { mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z };
              vertex.bitangent = { mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z };
            }

            if (mesh->HasTextureCoords(0)) {
              vertex.tex_coord = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
            } else {
              vertex.tex_coord = { 0.f, 0.f };
            }
          }

          for (uint32_t j = 0; j < mesh->mNumFaces; ++j) {
            aiFace& face = mesh->mFaces[j];
            OTHER_ASSERT(face.mNumIndices == 3, "Other Engine does not support untriangulated meshes. mesh [{}] face [{}] has [{}] indices", i, j, face.mNumIndices);
            index& idx = builder.indices.emplace_back();
            idx = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };

            triangle& triangle = builder.triangles[i].emplace_back();
            triangle.v0 = builder.vertices[idx.v0];
            triangle.v1 = builder.vertices[idx.v1];
            triangle.v2 = builder.vertices[idx.v2];

            triangle.centroid = (triangle.v0.position + triangle.v1.position + triangle.v2.position) / 3.0f;
          }

          vertex_count += mesh->mNumVertices;
          idx_count += submesh.idx_cnt;
        }
      }

      void traverse_assimp_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform, uint32_t level) {
        PROFILE_SECTION("model_importer::traverse_nodes");

        mesh_node& n = builder.nodes[node_idx];
        n.name = node->mName.C_Str();
        n.local_transform = mat4_from_ai_mat4(node->mTransformation);

        glm::mat4 transform = parent_transform * n.local_transform;
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
          uint32_t idx = node->mMeshes[i];
          submesh& submesh = builder.submeshes[idx];
          submesh.name = node->mName.C_Str();
          submesh.local_transform = n.local_transform;
          n.sub_meshes.push_back(idx);
        }

        uint32_t parent_node_idx = (uint32_t)builder.nodes.size() - 1;
        n.children.resize(node->mNumChildren);
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
          mesh_node& child = builder.nodes.emplace_back();
          size_t child_idx = builder.nodes.size() - 1;
          child.parent = parent_node_idx;
          builder.nodes[node_idx].children[i] = child_idx;
          traverse_assimp_nodes(node->mChildren[i], uint32_t(child_idx), builder, transform, level + 1);
        }
      }

      void process_assimp_materials(size_t idx, const aiScene* scene, model_builder& builder) {
        OTHER_ASSERT(scene != nullptr, "Scene is null");
        OTHER_ASSERT(idx < scene->mNumMaterials, "Material index out of bounds");

        aiMaterial* material = scene->mMaterials[idx];
        if (material == nullptr) {
          CORE_LOG_ERROR("Failed to get material : {}", idx);
          return;
        }

        aiString mat_name;
        if (material->Get(AI_MATKEY_NAME, mat_name)) {
          CORE_LOG_DEBUG("Material Name: {}", mat_name.C_Str());
        } else {
          CORE_LOG_DEBUG("Material Name: Unknown");
        }
        CORE_LOG_DEBUG("Material Index: {}", idx);
        uint32_t diff_text_count = material->GetTextureCount(aiTextureType_DIFFUSE);
        CORE_LOG_DEBUG("Diffuse Texture Count: {}", diff_text_count);

        opt<glm::vec4> albedo_col = std::nullopt;
        opt<float> emission = std::nullopt;

        aiColor3D col;
        aiColor3D emissive;

        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, col) == aiReturn_SUCCESS) {
          albedo_col = { col.r, col.g, col.b, 1.f };
        }
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == aiReturn_SUCCESS) {
          emission = emissive.r;
        }

        float roughness = 0.5f;
        float metalness = 0.0f;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS) {
          roughness = 0.5f;  // Default value
        }
        if (material->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS) {
          metalness = 0.0f;  // Default value
        }

        CORE_LOG_DEBUG("Material Properties:");
        CORE_LOG_DEBUG("  Albedo Color: {}, {}, {}, {}", col.r, col.g, col.b, albedo_col.has_value() ? albedo_col->a : 1.f);
        CORE_LOG_DEBUG("  Emissive Color: {}, {}, {}, {}", emissive.r, emissive.g, emissive.b, emission.has_value() ? emission.value() : 0.f);
        CORE_LOG_DEBUG("  Roughness: {}", roughness);
        CORE_LOG_DEBUG("  Metalness: {}", metalness);

        // bool has_albedo_map = material->GetTexture(aiTextureType_DIFFUSE, 0, &tex_path) == AI_SUCCESS;
        // bool fallback = !has_albedo_map;

        // Opt<MaterialTable::Texture> albedo_tex = std::nullopt;
        // Opt<MaterialTable::Texture> normal_tex = std::nullopt;
        // Opt<MaterialTable::Texture> roughness_tex = std::nullopt;

        // if (has_albedo_map) {
        //   if (const aiTexture* tex = scene->GetEmbeddedTexture(tex_path.C_Str())) {
        //     uint8_t* data = reinterpret_cast<uint8_t*>(tex->pcData);
        //     albedo_tex = MaterialTable::CreateTexture(4 * tex->mWidth * tex->mHeight, data, { tex->mWidth, tex->mHeight });
        //   }
        //   /// if texture not embedded, attempt loading from file if it exists
        //   else if (Path full_path = std::filesystem::absolute(Path{ tex_path.C_Str() }); Filesystem::FileExists(full_path)) {
        //     Ref<FileHandle> file = Filesystem::GetFile(full_path);
        //     if (file != nullptr) {
        //       std::vector<uint8_t> data = file->ReadBytes();
        //       if (data.empty()) {
        //         OE_ERROR("Failed to get texture  from file : {}", full_path);
        //         fallback = !albedo_col.has_value();
        //       } else {
        //         albedo_tex = MaterialTable::CreateTexture(data.size(), data.data(), { 1920.f, 1080.f });
        //       }
        //     } else {
        //       OE_ERROR("Failed to get file handle : {}", full_path);
        //       fallback = !albedo_col.has_value();
        //     }
        //   }
        //   /// file does not exist, fallback to color
        //   else {
        //     OE_DEBUG("    Could not load texture : {0}", tex_path.C_Str());
        //     fallback = !albedo_col.has_value();
        //   }
        // }

        // if (fallback) {
        //   albedo_tex = MaterialTable::CreateTexture(glm::vec4{ 1.f }, { 1920.f, 1080.f });
        // } else if (albedo_col.has_value()) {
        //   albedo_tex = MaterialTable::CreateTexture(albedo_col.value(), { 1920.f, 1080.f });
        // }

        // // Normal maps
        // bool has_normal_map = material->GetTexture(aiTextureType_NORMALS, 0, &tex_path) == AI_SUCCESS;
        // fallback = !has_normal_map;
        // if (has_normal_map) {
        //   if (const aiTexture* texture = scene->GetEmbeddedTexture(tex_path.C_Str())) {
        //     uint8_t* data = reinterpret_cast<uint8_t*>(texture->pcData);
        //     normal_tex = MaterialTable::CreateTexture(4 * texture->mWidth * texture->mHeight, data, { texture->mWidth, texture->mHeight });
        //   } else if (Path full_path = std::filesystem::absolute(Path{ tex_path.C_Str() }); Filesystem::FileExists(full_path)) {
        //     Ref<FileHandle> file = Filesystem::GetFile(full_path);
        //     if (file != nullptr) {
        //       std::vector<uint8_t> data = file->ReadBytes();
        //       if (data.empty()) {
        //         OE_ERROR("Failed to get texture  from file : {}", full_path);
        //         fallback = true;
        //       } else {
        //         normal_tex = MaterialTable::CreateTexture(data.size(), data.data(), { 1920.f, 1080.f });
        //       }
        //     } else {
        //       OE_ERROR("Failed to get file handle : {}", full_path);
        //       fallback = true;
        //     }
        //   } else {
        //     OE_DEBUG("    Could not load texture : {0}", tex_path.C_Str());
        //     fallback = true;
        //   }
        // }

        // if (fallback) {
        //   normal_tex = MaterialTable::CreateTexture(glm::vec4{ 0.5f, 0.5f, 1.f, 1.f }, { 1920.f, 1080.f });
        // }

        // // Roughness map
        // bool has_roughness_map = material->GetTexture(aiTextureType_SHININESS, 0, &tex_path) == AI_SUCCESS;
        // fallback = !has_roughness_map;
        // if (has_roughness_map) {
        //   if (const aiTexture* texture = scene->GetEmbeddedTexture(tex_path.C_Str())) {
        //     uint8_t* data = reinterpret_cast<uint8_t*>(texture->pcData);
        //     roughness_tex = MaterialTable::CreateTexture(4 * texture->mWidth * texture->mHeight, data, { texture->mWidth, texture->mHeight });
        //   } else if (Path full_path = std::filesystem::absolute(Path{ tex_path.C_Str() }); Filesystem::FileExists(full_path)) {
        //     Ref<FileHandle> file = Filesystem::GetFile(full_path);
        //     if (file != nullptr) {
        //       std::vector<uint8_t> data = file->ReadBytes();
        //       if (data.empty()) {
        //         OE_ERROR("Failed to get texture  from file : {}", full_path);
        //         fallback = true;
        //       } else {
        //         roughness_tex = MaterialTable::CreateTexture(data.size(), data.data(), { 1920.f, 1080.f });
        //       }
        //     } else {
        //       OE_ERROR("Failed to get file handle : {}", full_path);
        //       fallback = true;
        //     }
        //   } else {
        //     OE_DEBUG("    Could not load texture : {0}", tex_path.C_Str());
        //     fallback = true;
        //   }
        // }

        // if (fallback) {
        //   roughness_tex = MaterialTable::CreateTexture(glm::vec4{ 0.5f, 0.5f, 0.5f, 1.f }, { 1920.f, 1080.f });
        // }

        // OE_ASSERT(albedo_tex.has_value(), "Albedo texture is null");
        // OE_ASSERT(normal_tex.has_value(), "Normal texture is null");
        // OE_ASSERT(roughness_tex.has_value(), "Roughness texture is null");

        // Ref<MaterialTable> material_table = AssetManager::GetMaterialTable();
        // OE_ASSERT(material_table != nullptr, "Material table is null");

        // UUID mat_id = material_table->RegisterMaterial(albedo_tex.value(), normal_tex.value(), roughness_tex.value());
        // OE_DEBUG("    Material ID = {0}", mat_id);
        // return mat_id;
      }

    }  // namespace detail
  }  // namespace model_importer
}  // namespace other