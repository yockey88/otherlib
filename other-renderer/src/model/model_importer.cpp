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
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/fwd.hpp>
#include <mio/mmap.hpp>

#include "core/formatting.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "serialization/serialization.hpp"

#include "model/animation.hpp"
#include "model/skeleton.hpp"

#include "assimp/mesh.h"
#include "assimp/vector3.h"

namespace other {

  submesh& model_builder::new_submesh(const std::string_view name, uint32_t idx, uint32_t base_vert, uint32_t num_vertices, uint32_t base_idx, uint32_t num_faces) {
    submesh& submesh = submeshes.emplace_back();
    submesh.name = name;
    submesh.sub_mesh_id = idx;

    submesh.base_vertex = base_vert;
    submesh.vert_cnt = num_vertices;

    submesh.base_idx = base_idx;
    submesh.idx_cnt = num_faces * 3;
    return submesh;
  }

  void model_builder::build_triangle_list() {
    triangles.clear();
    triangles.reserve(indices.size() / 3);

    uint32_t tri_num = 0;
    for (const auto& idx : indices) {
      triangle& tri = triangles.emplace_back();
      tri.id = tri_num;

      vertex& v0 = vertices[tri.v0_id];
      v0.connected_triangles.emplace_back(tri.id);

      vertex& v1 = vertices[tri.v1_id];
      v1.connected_triangles.emplace_back(tri.id);

      vertex& v2 = vertices[tri.v2_id];
      v2.connected_triangles.emplace_back(tri.id);

      tri.v0_id = idx.v0;
      tri.v1_id = idx.v1;
      tri.v2_id = idx.v2;
      tri.centroid = (vertices[tri.v0_id].position + vertices[tri.v1_id].position + vertices[tri.v2_id].position) / 3.0f;
      tri_num++;
    }
  }

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
      ss << std::format("\n   - Submesh[{}] -- base_vertex: {}, base_idx: {}, idx_cnt: {}, vert_cnt: {}, name: {}", s, submesh.base_vertex, submesh.base_idx, submesh.idx_cnt, submesh.vert_cnt, submesh.name);
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

    void process_other_mesh_file(const filepath& file_path, model_builder& builder);
    void process_assimp_file(const filepath& file_path, model_builder& builder);

    void process_assimp_meshes(const aiScene* scene, model_builder& builder);
    void process_assimp_nodes(const aiScene* scene, model_builder& builder);
    void traverse_assimp_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform = glm::mat4(1.f), uint32_t level = 0);
    void process_assimp_skeleton(const aiScene* scene, const aiNode* node, skeleton& skel);
    void process_assimp_materials(const aiScene* scene, model_builder& builder);
    void process_assimp_animations(const aiScene* scene, model_builder& builder);

    model_builder load_model_data(const filepath& file_path) {
      PROFILE_SECTION("model_importer::load_model_data");

      /// \todo read import settings from config file or environment
      /// config_table& config = subsystem<config_manager>::get()->get_config("renderer");

      model_builder builder;
      OTHER_ASSERT(std::filesystem::exists(file_path), "Model file does not exist: {}", file_path.string());

      std::string extension = file_path.extension().string();
      if (extension == ".omesh") {
        // process_other_mesh_file(file_path, builder);
      } else if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" ||
                 extension == ".glb" || extension == ".dae" || extension == ".3ds") {
        process_assimp_file(file_path, builder);
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
      submesh.vert_cnt = static_cast<uint32_t>(vertices.size());
      submesh.idx_cnt = static_cast<uint32_t>(indices.size() * 3);
      submesh.sub_mesh_id = 0;
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

      builder.build_triangle_list();
      mesh_node& _ = builder.nodes.emplace_back();

      return builder;
    }

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
        submesh.idx_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);
        submesh.vert_cnt = serialization::read_value<uint32_t>(mesh_bytes, mesh_cursor);

        submesh.local_transform = serialization::read_value<glm::mat4>(mesh_bytes, mesh_cursor);

        submesh.bounds.min = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor);
        submesh.bounds.max = serialization::read_value<glm::vec3>(mesh_bytes, mesh_cursor);
        submesh.sub_mesh_id = serialization::read_value<natural_t>(mesh_bytes, mesh_cursor);

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
        triangle& tri = builder.triangles.emplace_back();
        tri.id = i;

        vertex& v0 = builder.vertices[tri.v0_id];
        v0.connected_triangles.emplace_back(tri.id);

        vertex& v1 = builder.vertices[tri.v1_id];
        v1.connected_triangles.emplace_back(tri.id);

        vertex& v2 = builder.vertices[tri.v2_id];
        v2.connected_triangles.emplace_back(tri.id);

        tri.v0_id = idx.v0;
        tri.v1_id = idx.v1;
        tri.v2_id = idx.v2;
        tri.centroid = (builder.vertices[tri.v0_id].position + builder.vertices[tri.v1_id].position + builder.vertices[tri.v2_id].position) / 3.0f;
        i++;
      }
    }

    void process_assimp_file(const filepath& file_path, model_builder& builder) {
      Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
      Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_STDOUT);

      std::string log_file = std::format("logs/{}-import-info.log", file_path.filename().stem().string());
      Assimp::DefaultLogger::create(log_file.c_str(), severity, aiDefaultLogStream_FILE);
      Assimp::DefaultLogger::get()->info("begin import of model: ", file_path.string());

      OTHER_ASSERT(std::filesystem::exists(file_path), "{} does not exist", file_path.string());

      uint32_t flags =
        aiProcess_CalcTangentSpace  // Create binormals/tangents just in case
        | aiProcess_Triangulate     // Make sure we're triangles
        | aiProcess_SortByPType     // Split meshes by primitive type
        | aiProcess_GenNormals      // Make sure we have legit normals
        | aiProcess_GenUVCoords     // Convert UVs if required
                                    //		| aiProcess_OptimizeGraph
        | aiProcess_OptimizeMeshes  // Batch draws where possible
        // | aiProcess_JoinIdenticalVertices
        | aiProcess_LimitBoneWeights       // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
        | aiProcess_ValidateDataStructure  // Validation
        | aiProcess_GlobalScale            // e.g. convert cm to m for fbx import (and other formats where cm is native)
        ;

      Assimp::Importer importer;
      importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
      const aiScene* scene = nullptr;
      {
        PROFILE_SECTION("model_importer::load_model_data--read-file");

        scene = importer.ReadFile(file_path.string(), flags);
        if (scene == nullptr) {
          CORE_LOG_ERROR("Failed to load model : {} [{}]", file_path.string(), importer.GetErrorString());
          return;
        }

#define DEBUG_ASSIMP_IMPORTING
#ifdef DEBUG_ASSIMP_IMPORTING
        if (scene->mMetaData != nullptr) {
          std::stringstream ss;
          ss << "Model Metadata:\n";
          for (unsigned int i = 0; i < scene->mMetaData->mNumProperties; ++i) {
            aiString key = scene->mMetaData->mKeys[i];
            aiMetadataEntry entry = scene->mMetaData->mValues[i];

            ss << std::format(" - Key: {}\n", key.C_Str());

            switch (entry.mType) {
              case AI_BOOL:
                ss << std::format("   Value: {}\n", (*(bool*)entry.mData ? "true" : "false"));
                break;
              case AI_INT32:
                ss << std::format("   Value: {}\n", *(int32_t*)entry.mData);
                break;
              case AI_UINT64:
                ss << std::format("   Value: {}\n", *(uint64_t*)entry.mData);
                break;
              case AI_FLOAT:
                ss << std::format("   Value: {}\n", *(float*)entry.mData);
                break;
              case AI_DOUBLE:
                ss << std::format("   Value: {}\n", *(double*)entry.mData);
                break;
              case AI_AISTRING:
                ss << std::format("   Value: {}\n", ((aiString*)entry.mData)->C_Str());
                break;
              case AI_AIVECTOR3D: {
                aiVector3D* vec = (aiVector3D*)entry.mData;
                ss << std::format("   Value: ({0}, {1}, {2})\n", vec->x, vec->y, vec->z);
                break;
              }
              default: ss << "   Value: [unknown type]\n"; break;
            }
          }
#endif

          CORE_LOG_DEBUG("{}", ss.str());
        }
      }

      if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
        CORE_LOG_ERROR("Failed to load model : {}\n[ASSIMP ERROR : {}]", file_path.string(), importer.GetErrorString());
        return;
      }

      CORE_LOG_DEBUG("Model loaded successfully: {}", file_path.string());
      Assimp::DefaultLogger::get()->info("Model loaded successfully: ", file_path.string());

      CORE_LOG_DEBUG("Model has {} meshes, {} materials, {} textures, {} animations, and {} lights", scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures, scene->mNumAnimations, scene->mNumLights);

      builder.global_transform = mat4_from_ai_mat4(scene->mRootNode->mTransformation);
      builder.inverse_global_transform = glm::inverse(builder.global_transform);

      process_assimp_skeleton(scene, scene->mRootNode, builder.skel);
      process_assimp_meshes(scene, builder);
      process_assimp_nodes(scene, builder);
      process_assimp_materials(scene, builder);
      process_assimp_animations(scene, builder);

      for (auto& infl : builder.skel.bone_influence) {
        infl.normalize();
      }

      for (uint32_t v = 0; v < builder.vertices.size(); ++v) {
        const bone_influence& infl = builder.skel.bone_influence[v];
        vertex& vert = builder.vertices[v];
        for (uint32_t b = 0; b < bone_influence::kMaxBones; ++b) {
          vert.bone_ids[b] = infl.bone_ids[b];
          vert.bone_weights[b] = infl.weights[b];
        }
      }

      // builder.skel.model_space_rest_transforms.resize(builder.skel.bones.size(), glm::mat4(1.f));
      // builder.skel.model_space_rest_inverse_transforms.resize(builder.skel.bones.size(), glm::mat4(1.f));

      // builder.skel.model_space_rest_transforms[0] =
      //   glm::translate(glm::mat4(1.f), builder.skel.bone_positions[0]) *
      //   glm::toMat4(builder.skel.bone_rotations[0]) *
      //   glm::scale(glm::mat4(1.f), builder.skel.bone_scales[0]);

      // builder.skel.model_space_rest_inverse_transforms[0] = glm::inverse(builder.skel.model_space_rest_transforms[0]);
      // for (uint32_t i = 1; i < builder.skel.bones.size(); ++i) {
      //   glm::mat4 parent_transform = glm::mat4(1.f);
      //   if (builder.skel.parent_ids[i] != -1) {
      //     parent_transform = builder.skel.model_space_rest_transforms[builder.skel.parent_ids[i]];
      //   }

      //   builder.skel.model_space_rest_transforms[i] =
      //     parent_transform *
      //     glm::translate(glm::mat4(1.f), builder.skel.bone_positions[i]) *
      //     glm::toMat4(builder.skel.bone_rotations[i]) *
      //     glm::scale(glm::mat4(1.f), builder.skel.bone_scales[i]);
      //   builder.skel.model_space_rest_inverse_transforms[i] = glm::inverse(builder.skel.model_space_rest_transforms[i]);
      // }

      for (const auto& node : builder.nodes) {
        CORE_LOG_DEBUG("Node '{}' has {} submeshes", node.name, node.sub_meshes.size());
        for (const auto& sm_idx : node.sub_meshes) {
          const submesh& sm = builder.submeshes[sm_idx];
          CORE_LOG_DEBUG(" - Submesh[{}] name: '{}'", sm_idx, sm.name);
        }
      }
    }

    void process_mesh(const aiScene* scene, const aiMesh* mesh, model_builder& builder, uint32_t i, uint32_t& vertex_count, uint32_t& idx_count) {
      OTHER_ASSERT(scene != nullptr, "Scene is null");
      OTHER_ASSERT(mesh != nullptr, "Mesh is null");

      std::string name = mesh->mName.C_Str();
      CORE_LOG_DEBUG("Processing mesh[{}] with name '{}'", i, name);

      uint32_t idx = static_cast<uint32_t>(builder.submeshes.size());
      submesh& submesh = builder.new_submesh(name, idx, vertex_count, mesh->mNumVertices, idx_count, mesh->mNumFaces);

      bounding_box& bounds = submesh.bounds;
      bounds.min = glm::vec3(std::numeric_limits<float>::max());
      bounds.max = glm::vec3(std::numeric_limits<float>::lowest());
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

        vertex.bone_ids = { 0, 0, 0, 0 };
        vertex.bone_weights = { 0.f, 0.f, 0.f, 0.f };

        bounds.min.x = glm::min(bounds.min.x, vertex.position.x);
        bounds.min.y = glm::min(bounds.min.y, vertex.position.y);
        bounds.min.z = glm::min(bounds.min.z, vertex.position.z);
        bounds.max.x = glm::max(bounds.max.x, vertex.position.x);
        bounds.max.y = glm::max(bounds.max.y, vertex.position.y);
        bounds.max.z = glm::max(bounds.max.z, vertex.position.z);
      }
      for (uint32_t j = 0; j < mesh->mNumFaces; ++j) {
        aiFace& face = mesh->mFaces[j];
        OTHER_ASSERT(face.mNumIndices == 3, "Other Engine does not support untriangulated meshes. mesh [{}] face [{}] has [{}] indices", i, j, face.mNumIndices);
        index& idx = builder.indices.emplace_back();
        idx = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };
      }
      builder.build_triangle_list();

      vertex_count += mesh->mNumVertices;
      idx_count += submesh.idx_cnt;
    }

    void process_assimp_meshes(const aiScene* scene, model_builder& builder) {
      PROFILE_SECTION("model_importer::load_model_data--process-submeshes");

      uint32_t vertex_count = 0;
      uint32_t idx_count = 0;

      for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!mesh->HasPositions()) {
          CORE_LOG_WARN("Mesh index {} with name '{}' has no vertex positions - skipping import!", i, mesh->mName.C_Str());
        }
        if (!mesh->HasNormals()) {
          CORE_LOG_WARN("Mesh index {} with name '{}' has no vertex normals, and they could not be computed - skipping import!", i, mesh->mName.C_Str());
        }

        const bool skip = !mesh->HasPositions() || !mesh->HasNormals();
        if (skip) {
          continue;
        }

        process_mesh(scene, mesh, builder, i, vertex_count, idx_count);
      }

      builder.bounds = bounding_box::empty;
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

      builder.skel.bone_influence.resize(builder.vertices.size());
      for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        submesh& submesh = builder.submeshes.at(m);

        if (mesh->mNumBones == 0) {
          submesh.rigged = false;
          continue;
        }

        submesh.rigged = true;

        for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
          const aiBone* bone = mesh->mBones[b];

          bool has_nonzero_weights = false;
          for (size_t j = 0; j < bone->mNumWeights; j++) {
            if (bone->mWeights[j].mWeight > 0.000001f) {
              has_nonzero_weights = true;
            }
          }

          if (!has_nonzero_weights) {
            continue;
          }

          std::string bone_name = bone->mName.C_Str();
          int32_t bone_idx = builder.skel.get_bone_index(bone_name);
          if (bone_idx == -1) {
            continue;
          }

          aiMatrix4x4 ai_offset_matrix = bone->mOffsetMatrix;
          glm::mat4 offset_matrix = mat4_from_ai_mat4(ai_offset_matrix);

          builder.skel.bones[bone_idx].offset_matrix = offset_matrix;
          for (uint32_t w = 0; w < bone->mNumWeights; ++w) {
            const aiVertexWeight& weight = bone->mWeights[w];
            uint32_t vertex_id = submesh.base_vertex + weight.mVertexId;
            float weight_value = weight.mWeight;

            builder.skel.bone_influence[vertex_id].add_bone_data(bone_idx, weight_value);
          }
        }
      }
    }

    void process_assimp_nodes(const aiScene* scene, model_builder& builder) {
      mesh_node& root = builder.nodes.emplace_back();
      traverse_assimp_nodes(scene->mRootNode, 0, builder);

      /// check if we created a bone from an animation channel and need to update the skeleton data and unrigged submeshes
      for (auto& node : builder.nodes) {
        auto bone_it = std::ranges::find_if(builder.skel.bones, [&](const bone& b) { return b.name == node.name; });
        if (bone_it == builder.skel.bones.end()) {
          continue;
        }

        int32_t bone_idx = builder.skel.get_bone_index(node.name);
        OTHER_ASSERT(bone_idx >= 0, "Bone '{}' not found in skeleton", node.name);

        bone& b = builder.skel.bones[bone_idx];
        b.offset_matrix = glm::mat3(1.0);

        for (auto smidx : node.sub_meshes) {
          submesh& submesh = builder.submeshes[smidx];
          submesh.rigged = true;

          CORE_LOG_DEBUG("Processing bone '{}' for submesh '{}'", node.name, submesh.name);
          CORE_LOG_DEBUG(" - Rigging mesh[{}] with 'bone' '{}'", smidx, node.name);
          for (uint32_t v = 0; v < submesh.vert_cnt; ++v) {
            uint32_t vertex_id = submesh.base_vertex + v;

            for (uint32_t bidx = 0; bidx < bone_influence::kMaxBones; ++bidx) {
              builder.skel.bone_influence[vertex_id].add_bone_data(bone_idx, 1.f);
            }
          }
        }
      }
    }

    bool node_contains_mesh(const aiNode* node) {
      if (node == nullptr) {
        return false;
      }

      if (node->mNumMeshes > 0) {
        return true;
      }

      for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; ++nodeIndex) {
        if (node_contains_mesh(node->mChildren[nodeIndex])) {
          return true;
        }
      }

      return false;
    }

    bool node_contains_bone(const skeleton& skel, const aiNode* node) {
      OTHER_ASSERT(node != nullptr, "aiNode is null");

      if (std::ranges::any_of(skel.bone_names, [&node](const auto& b) { return b == node->mName.C_Str(); })) {
        return true;
      }

      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        if (node_contains_bone(skel, node->mChildren[i])) {
          return true;
        }
      }

      return false;
    }

    void traverse_bone(skeleton& skel, const aiScene* scene, const aiNode* node, uint32_t parent_idx) {
      aiVector3D ai_scaling;
      aiQuaternion ai_rotation;
      aiVector3D ai_position;
      node->mTransformation.Decompose(ai_scaling, ai_rotation, ai_position);

      glm::vec3 translation = glm::vec3(ai_position.x, ai_position.y, ai_position.z);
      glm::quat rotation = glm::quat(ai_rotation.w, ai_rotation.x, ai_rotation.y, ai_rotation.z);
      glm::vec3 scale = glm::vec3(ai_scaling.x, ai_scaling.y, ai_scaling.z);

      // check uniform scale
      if (fabs(scale.x - scale.y) > 0.00001f || fabs(scale.x - scale.z) > 0.00001f) {
        CORE_LOG_WARN("Animation", "Bone '{}' has non-uniform scale ({}, {}, {}). This is not supported by the animation system, and will result in incorrect animations!", node->mName.C_Str(), scale.x, scale.y, scale.z);
      }

      /// should we create the bone here instead ?
      uint32_t bone_idx = skel.add_bone_data(node->mName.C_Str(), parent_idx, translation, rotation, scale);
      for (uint32_t node_idx = 0; node_idx < node->mNumChildren; ++node_idx) {
        skel.children_ids[bone_idx].push_back(node_idx);
        aiNode* child_node = node->mChildren[node_idx];

        if (node_contains_bone(skel, child_node)) {
          traverse_bone(skel, scene, child_node, bone_idx);
        } else {
          /// no op
        }
      }
    }

    void traverse_node(skeleton& skel, const aiScene* scene, const aiNode* node, uint32_t parent_idx = -1) {
      OTHER_ASSERT(node != nullptr, "aiNode is null");
      CORE_LOG_DEBUG("Traversing node: {}", node->mName.C_Str());

      uint32_t num_bone_children = 0;
      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        if (node_contains_bone(skel, node->mChildren[i])) {
          ++num_bone_children;
        }
      }

      bool is_multi_root_parent = num_bone_children > 1;

      // Sometimes there is an "Armature" node or the like that is the parent of the skeleton.
      // This node is not actually a bone, but we need to treat it as such so that its transform is not overlooked when we come
      // to converting the bone transforms to model space.  If this node has identity transform, we can ignore it.
      // note: As of Assimp 6.0 this appears to no longer be needed
      bool armature_node = (num_bone_children == 1) && !node->mTransformation.IsIdentity() && !node_contains_mesh(node);
      bool is_bone = std::ranges::any_of(skel.bone_names, [&node](const auto& b) { return b == node->mName.C_Str(); });

      if (is_bone || is_multi_root_parent || armature_node) {
        traverse_bone(skel, scene, node, parent_idx);
      } else {
        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
          traverse_node(skel, scene, node->mChildren[i], parent_idx);
        }
      }
    }

    void traverse_assimp_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform, uint32_t level) {
      PROFILE_SECTION("model_importer::traverse_nodes");

      mesh_node& n = builder.nodes[node_idx];
      n.name = node->mName.C_Str();
      n.local_transform = mat4_from_ai_mat4(node->mTransformation);
      n.inverse_local_transform = glm::inverse(n.local_transform);
      CORE_LOG_DEBUG("{}Node[{}] name: '{}', num_children: {}, num_meshes: {}", std::string(level * 2, ' '), node_idx, n.name, node->mNumChildren, node->mNumMeshes);

      glm::mat4 transform = parent_transform * n.local_transform;
      for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        uint32_t idx = node->mMeshes[i];
        submesh& submesh = builder.submeshes[idx];

        submesh.node_id = node_idx;
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

    void process_assimp_skeleton(const aiScene* scene, const aiNode* node, skeleton& skel) {
      for (uint32_t mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index) {
        const aiMesh* mesh = scene->mMeshes[mesh_index];

        for (uint32_t bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx) {
          CORE_LOG_DEBUG(" - Creating bone from mesh bone: {}", mesh->mBones[bone_idx]->mName.C_Str());
          skel.bone_names.emplace(mesh->mBones[bone_idx]->mName.C_Str());
        }
      }

      traverse_node(skel, scene, node, -1);

      for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx) {
        aiAnimation* animation = scene->mAnimations[anim_idx];

        for (uint32_t channel_idx = 0; channel_idx < animation->mNumChannels; ++channel_idx) {
          const aiNodeAnim* node_anim = animation->mChannels[channel_idx];

          if (node_anim->mNumPositionKeys == 0 && node_anim->mNumRotationKeys == 0 && node_anim->mNumScalingKeys == 0) {
            continue;
          }

          std::string channel_name = node_anim->mNodeName.C_Str();
          if (std::ranges::any_of(skel.bones, [&channel_name](const auto& b) { return b.name == channel_name; })) {
            continue;
          }

          const aiNode* node = scene->mRootNode;
          if (node && node->mName != node_anim->mNodeName) {
            node = node->FindNode(node_anim->mNodeName.C_Str());
          }
          if (node == nullptr) {
            continue;
          }

          aiVector3D scaling;
          aiQuaternion rotation;
          aiVector3D position;
          node->mTransformation.Decompose(scaling, rotation, position);
          skel.bone_names.emplace(channel_name);

          glm::vec3 translation = glm::vec3(position.x, position.y, position.z);
          glm::quat rot = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
          glm::vec3 scale = glm::vec3(scaling.x, scaling.y, scaling.z);

          CORE_LOG_DEBUG(" - Creating bone from animation channel: {}", channel_name);
          skel.add_bone_data(channel_name, -1, translation, rot, scale);
        }
      }

      for (uint32_t i = 0; i < skel.bones.size(); ++i) {
        skel.calculate_final_transform(i);
      }
    }

    void process_assimp_materials(const aiScene* scene, model_builder& builder) {
      OTHER_ASSERT(scene != nullptr, "Scene is null");

      if (scene->mNumMaterials == 0) {
        CORE_LOG_DEBUG("No materials found in model");
        return;
      }

      builder.materials.reserve(scene->mNumMaterials);

      for (uint32_t idx = 0; idx < scene->mNumMaterials; ++idx) {
        aiMaterial* ai_material = scene->mMaterials[idx];
        if (ai_material == nullptr) {
          CORE_LOG_ERROR("Failed to get material : {}", idx);
          return;
        }

        material& mat = builder.materials.emplace_back();

        aiString mat_name;
        if (ai_material->Get(AI_MATKEY_NAME, mat_name)) {
          CORE_LOG_DEBUG("Material Name: {}", mat_name.C_Str());
        } else {
          CORE_LOG_DEBUG("Material Name: Unknown");
        }
        uint32_t diff_text_count = ai_material->GetTextureCount(aiTextureType_DIFFUSE);

        /// #a69f74 donkey brown
        constexpr static glm::vec4 kDefaultColor = glm::vec4(166.f / 255, 159.f / 255, 116.f / 255, 1.f);

        aiColor3D ai_ambient;
        aiColor3D ai_diffuse;
        aiColor3D ai_specular;

        if (ai_material->Get(AI_MATKEY_COLOR_AMBIENT, ai_ambient) == aiReturn_SUCCESS) {
          mat.ambient_color = { ai_ambient.r, ai_ambient.g, ai_ambient.b };
        } else {
          mat.ambient_color = glm::vec3(kDefaultColor);
        }

        if (ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_diffuse) == aiReturn_SUCCESS) {
          mat.diffuse_color = { ai_diffuse.r, ai_diffuse.g, ai_diffuse.b };
        } else {
          mat.diffuse_color = glm::vec3(kDefaultColor);
        }

        if (ai_material->Get(AI_MATKEY_COLOR_SPECULAR, ai_specular) == aiReturn_SUCCESS) {
          mat.specular_color = { ai_specular.r, ai_specular.g, ai_specular.b };
        } else {
          mat.specular_color = glm::vec3(kDefaultColor);
        }

        if (ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness) != aiReturn_SUCCESS) {
          mat.roughness = 1.0f;
        }
        if (ai_material->Get(AI_MATKEY_REFLECTIVITY, mat.metalness) != aiReturn_SUCCESS) {
          mat.metalness = 0.0f;
        }

        CORE_LOG_DEBUG(" - Material[{}] name: '{}', diffuse_textures: {}, ambient_color: {}, diffuse_color: {}, specular_color: {}, roughness: {}, metalness: {}", idx, mat_name.C_Str(), diff_text_count, mat.ambient_color, mat.diffuse_color, mat.specular_color, mat.roughness, mat.metalness);
      }
    }

    void process_assimp_animations(const aiScene* scene, model_builder& builder) {
      PROFILE_SECTION("model_importer::load_model_data--process-animations");

      if (scene->mNumAnimations == 0) {
        CORE_LOG_DEBUG("No animations found in model");
        return;
      }

      builder.animations.reserve(scene->mNumAnimations);

      CORE_LOG_DEBUG("Processing {} animations", scene->mNumAnimations);
      for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = scene->mAnimations[i];
        OTHER_ASSERT(ai_anim != nullptr, "Failed to get animation");

        animation& anim = builder.animations.emplace_back();
        anim.name = ai_anim->mName.C_Str();
        anim.duration = ai_anim->mDuration;
        anim.ticks_per_second = ai_anim->mTicksPerSecond != 0.0 ? ai_anim->mTicksPerSecond : 25.0;
        CORE_LOG_DEBUG(" - Animation[{}] name: '{}', duration: {}, ticks_per_second: {}, num_channels: {}", i, anim.name, anim.duration, anim.ticks_per_second, ai_anim->mNumChannels);

        for (uint32_t j = 0; j < ai_anim->mNumChannels; ++j) {
          aiNodeAnim* channel = ai_anim->mChannels[j];
          OTHER_ASSERT(channel != nullptr, "Failed to get animation channel");

          animation_channel& anim_channel = anim.channels.emplace_back();
          anim_channel.node_name = channel->mNodeName.C_Str();

          for (uint32_t k = 0; k < channel->mNumPositionKeys; ++k) {
            aiVectorKey pos_key = channel->mPositionKeys[k];
            anim_channel.position_keys.push_back(translation_key<glm::vec3>{
              .index = k,
              .time = pos_key.mTime,
              .value = glm::vec3(pos_key.mValue.x, pos_key.mValue.y, pos_key.mValue.z),
            });
          }

          for (uint32_t k = 0; k < channel->mNumRotationKeys; ++k) {
            aiQuatKey rot_key = channel->mRotationKeys[k];
            anim_channel.rotation_keys.push_back(translation_key<glm::quat>{
              .index = k,
              .time = rot_key.mTime,
              .value = glm::quat(rot_key.mValue.w, rot_key.mValue.x, rot_key.mValue.y, rot_key.mValue.z),
            });
          }

          for (uint32_t k = 0; k < channel->mNumScalingKeys; ++k) {
            aiVectorKey scale_key = channel->mScalingKeys[k];
            anim_channel.scale_keys.push_back(translation_key<glm::vec3>{
              .index = k,
              .time = scale_key.mTime,
              .value = glm::vec3(scale_key.mValue.x, scale_key.mValue.y, scale_key.mValue.z),
            });
          }
          CORE_LOG_DEBUG("   - Channel[{}] node_name: '{}', num_position_keys: {}, num_rotation_keys: {}, num_scale_keys: {}", j, anim_channel.node_name, channel->mNumPositionKeys, channel->mNumRotationKeys, channel->mNumScalingKeys);
        }
      }
    }

  }  // namespace model_importer
}  // namespace other