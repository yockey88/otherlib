/**
 * \file model/model_importer.cpp
 **/
#include "model/model_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "assimp/matrix4x4.h"

namespace other {
  namespace model_importer {

    void traverse_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform = glm::mat4(1.f), uint32_t level = 0);
    void process_materials(size_t idx, const aiScene* scene, model_builder& builder);

    model_builder load_model_data(const filepath& file_path) {
      PROFILE_SECTION("model_importer::load_model_data");

      OTHER_ASSERT(std::filesystem::exists(file_path), "{} does not exist", file_path.string());
      model_builder builder;
      Assimp::Importer importer;

      uint32_t flags =
        aiProcess_CalcTangentSpace |  // Create binormals/tangents just in case
        aiProcess_Triangulate |       // Make sure we're triangles
        aiProcess_SortByPType |       // Split meshes by primitive type
        aiProcess_GenNormals |        // Make sure we have legit normals
        aiProcess_GenUVCoords |       // Convert UVs if required
        aiProcess_OptimizeGraph |
        aiProcess_RemoveRedundantMaterials |  // remove redundant materials
        aiProcess_FindDegenerates |           // remove degenerated polygons from the import
        aiProcess_FindInvalidData |           // detect invalid model data, such as invalid normal vectors
        aiProcess_TransformUVCoords |         // preprocess UV transformations (scaling, translation ...)
        aiProcess_FindInstances |             // search for instanced meshes and remove them by references to one master
        // aiProcess_SplitByBoneCount |          // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
        aiProcess_OptimizeMeshes |  // Batch draws where possible
        aiProcess_JoinIdenticalVertices |
        // aiProcess_LimitBoneWeights |       // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
        aiProcess_ValidateDataStructure |  // Validation
        aiProcess_GlobalScale;             // e.g. convert cm to m for fbx import (and other formats where cm is native)
      const aiScene* scene = importer.ReadFile(file_path.string(), flags);
      if (scene == nullptr) {
        CORE_LOG_ERROR("Failed to load model : {} [{}]", file_path.string(), importer.GetErrorString());
        return {};
      }

      if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
        CORE_LOG_ERROR("Failed to load model : {}\n[ASSIMP ERROR : {}]", file_path.string(), importer.GetErrorString());
        return {};
      }

      // ProcessMaterials(scene);
      {
        PROFILE_SECTION("model_importer::load_model_data--process-submeshes");

        uint32_t vertex_count = 0;
        uint32_t idx_count = 0;

        for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
          aiMesh* mesh = scene->mMeshes[i];
          OTHER_ASSERT(mesh != nullptr, "Failed to get mesh");
          if (!mesh->HasPositions()) {
            CORE_LOG_ERROR("Mesh has no positions");
            throw std::runtime_error("Mesh has no positions");
          }

          if (!mesh->HasNormals()) {
            CORE_LOG_ERROR("Mesh has no normals");
            throw std::runtime_error("Mesh has no normals");
          }

          submesh& submesh = builder.submeshes.emplace_back();
          submesh.sub_mesh_id = i;

          submesh.base_vertex = vertex_count;
          submesh.base_idx = idx_count;

          submesh.material_id = mesh->mMaterialIndex;
          submesh.vert_cnt = mesh->mNumVertices;
          submesh.idx_cnt = mesh->mNumFaces * 3;

          submesh.name = mesh->mName.C_Str();

          // OE_DEBUG("Submesh [{}] : submesh id = {} \\ num verts = {} (base = {}) ", submesh.model_name, submesh.sub_mesh_id.Get(), submesh.vert_cnt, submesh.base_vertex);

          for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
            vertex& vertex = builder.vertices.emplace_back();
            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

            if (mesh->HasTangentsAndBitangents()) {
              vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
              vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
            }

            if (mesh->HasTextureCoords(0)) {
              vertex.tex_coord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            } else {
              vertex.tex_coord = { 0.f, 0.f };
            }
          }

          for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            OTHER_ASSERT(face.mNumIndices == 3, "Other Engine does not support untriangulated meshes");
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

      mesh_node& _ = builder.nodes.emplace_back();
      traverse_nodes(scene->mRootNode, 0, builder);

      for (const auto& submesh : builder.submeshes) {
        bounding_box submesh_bounds = submesh.bounds;
        glm::vec3 min = glm::vec3(submesh.transform * glm::vec4(submesh_bounds.min, 1.0f));
        glm::vec3 max = glm::vec3(submesh.transform * glm::vec4(submesh_bounds.max, 1.0f));

        builder.bounds.min.x = glm::min(builder.bounds.min.x, min.x);
        builder.bounds.min.y = glm::min(builder.bounds.min.y, min.y);
        builder.bounds.min.z = glm::min(builder.bounds.min.z, min.z);
        builder.bounds.max.x = glm::max(builder.bounds.max.x, max.x);
        builder.bounds.max.y = glm::max(builder.bounds.max.y, max.y);
        builder.bounds.max.z = glm::max(builder.bounds.max.z, max.z);
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
      submesh.idx_cnt = static_cast<uint32_t>(indices.size());
      submesh.sub_mesh_id = 0;
      submesh.material_id = 0;
      submesh.transform = glm::mat4(1.0f);
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

    void traverse_nodes(const aiNode* node, uint32_t node_idx, model_builder& builder, const glm::mat4& parent_transform, uint32_t level) {
      PROFILE_SECTION("model_importer::traverse_nodes");

      mesh_node& n = builder.nodes[node_idx];
      n.name = node->mName.C_Str();
      n.local_transform = mat4_from_ai_mat4(node->mTransformation);

      glm::mat4 transform = parent_transform * n.local_transform;
      for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        uint32_t idx = node->mMeshes[i];
        submesh& submesh = builder.submeshes[idx];
        // submesh.model_name = node->mName.C_Str();
        submesh.transform = transform;
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
        traverse_nodes(node->mChildren[i], uint32_t(child_idx), builder, transform, level + 1);
      }
    }

    void process_materials(size_t idx, const aiScene* scene, model_builder& builder) {
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

  }  // namespace model_importer
}  // namespace other