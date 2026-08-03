/**
 * \file model/model_importer.cpp
 **/
#include "model/model_importer.hpp"

#include <cmath>
#include <format>
#include <limits>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  namespace {

    /// gates the very chatty assimp scene-metadata dump; flip on when debugging an import
    constexpr bool kDebugAssimpImporting = false;

    struct assimp_import_ctx {
      constexpr static uint32_t kSkipped = 0xFFFFFFFF;

      const aiScene* scene = nullptr;
      ostd::vector<uint32_t> submesh_of_mesh;  // [assimp mesh index] -> model_data submesh index, or kSkipped
      ostd::vector<std::string> warnings;

      void warn(std::string message) {
        CORE_LOG_WARN("{}", message);
        warnings.push_back(std::move(message));
      }
    };

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

    void log_scene_metadata(const aiScene* scene) {
      if (scene->mMetaData == nullptr) {
        return;
      }

      std::string dump = "Model Metadata:\n";
      for (unsigned int i = 0; i < scene->mMetaData->mNumProperties; ++i) {
        const aiString& key = scene->mMetaData->mKeys[i];
        const aiMetadataEntry& entry = scene->mMetaData->mValues[i];

        dump += std::format(" - Key: {}\n", key.C_Str());
        switch (entry.mType) {
          case AI_BOOL: dump += std::format("   Value: {}\n", *static_cast<bool*>(entry.mData)); break;
          case AI_INT32: dump += std::format("   Value: {}\n", *static_cast<int32_t*>(entry.mData)); break;
          case AI_UINT64: dump += std::format("   Value: {}\n", *static_cast<uint64_t*>(entry.mData)); break;
          case AI_FLOAT: dump += std::format("   Value: {}\n", *static_cast<float*>(entry.mData)); break;
          case AI_DOUBLE: dump += std::format("   Value: {}\n", *static_cast<double*>(entry.mData)); break;
          case AI_AISTRING: dump += std::format("   Value: {}\n", static_cast<aiString*>(entry.mData)->C_Str()); break;
          case AI_AIVECTOR3D: {
            const aiVector3D* vec = static_cast<aiVector3D*>(entry.mData);
            dump += std::format("   Value: ({}, {}, {})\n", vec->x, vec->y, vec->z);
            break;
          }
          default: dump += "   Value: [unknown type]\n"; break;
        }
      }

      CORE_LOG_DEBUG("{}", dump);
    }

    bool node_contains_mesh(const aiNode* node) {
      if (node == nullptr) {
        return false;
      }

      if (node->mNumMeshes > 0) {
        return true;
      }

      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        if (node_contains_mesh(node->mChildren[i])) {
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

    void traverse_bone(skeleton& skel, const aiNode* node, uint32_t parent_idx, ostd::vector<std::string>& warnings) {
      aiVector3D ai_scaling;
      aiQuaternion ai_rotation;
      aiVector3D ai_position;
      node->mTransformation.Decompose(ai_scaling, ai_rotation, ai_position);

      glm::vec3 translation = glm::vec3(ai_position.x, ai_position.y, ai_position.z);
      glm::quat rotation = glm::quat(ai_rotation.w, ai_rotation.x, ai_rotation.y, ai_rotation.z);
      glm::vec3 scale = glm::vec3(ai_scaling.x, ai_scaling.y, ai_scaling.z);

      if (std::abs(scale.x - scale.y) > 0.00001f || std::abs(scale.x - scale.z) > 0.00001f) {
        std::string w = std::format("bone '{}' has non-uniform scale ({}, {}, {}) - the animation system does not support this and animations will be incorrect", node->mName.C_Str(), scale.x, scale.y, scale.z);
        CORE_LOG_WARN("{}", w);
        warnings.push_back(std::move(w));
      }

      uint32_t bone_idx = skel.add_bone_data(node->mName.C_Str(), parent_idx, translation, rotation, scale);
      for (uint32_t node_idx = 0; node_idx < node->mNumChildren; ++node_idx) {
        /// \todo doc 03: this records the aiNode child slot, not a bone index - dies with the skeleton rework
        skel.children_ids[bone_idx].push_back(node_idx);
        aiNode* child_node = node->mChildren[node_idx];

        if (node_contains_bone(skel, child_node)) {
          traverse_bone(skel, child_node, bone_idx, warnings);
        }
      }
    }

    void traverse_node(skeleton& skel, const aiNode* node, uint32_t parent_idx, ostd::vector<std::string>& warnings) {
      OTHER_ASSERT(node != nullptr, "aiNode is null");

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
        traverse_bone(skel, node, parent_idx, warnings);
      } else {
        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
          traverse_node(skel, node->mChildren[i], parent_idx, warnings);
        }
      }
    }

    void process_assimp_skeleton(const aiScene* scene, skeleton& skel, ostd::vector<std::string>& warnings) {
      PROFILE_SECTION("model_importer::import_assimp--process-skeleton");

      for (uint32_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        const aiMesh* mesh = scene->mMeshes[mesh_idx];
        for (uint32_t bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx) {
          CORE_LOG_DEBUG(" - Creating bone from mesh bone: {}", mesh->mBones[bone_idx]->mName.C_Str());
          skel.bone_names.emplace(mesh->mBones[bone_idx]->mName.C_Str());
        }
      }

      traverse_node(skel, scene->mRootNode, static_cast<uint32_t>(-1), warnings);

      /// animation channels can target nodes that no mesh rigs; they still need bones so the clips can drive them
      for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx) {
        const aiAnimation* animation = scene->mAnimations[anim_idx];

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
          if (node->mName != node_anim->mNodeName) {
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
          skel.add_bone_data(channel_name, static_cast<uint32_t>(-1), translation, rot, scale);
        }
      }

      for (uint32_t i = 0; i < skel.bones.size(); ++i) {
        skel.calculate_final_transform(i);
      }
    }

    /// pass 1 - decide accept/skip per assimp mesh; every later pass maps mesh indices through this table
    void map_accepted_meshes(assimp_import_ctx& ctx) {
      const aiScene* scene = ctx.scene;
      ctx.submesh_of_mesh.resize(scene->mNumMeshes, assimp_import_ctx::kSkipped);

      uint32_t next_submesh = 0;
      for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!mesh->HasPositions() || !mesh->HasNormals()) {
          ctx.warn(std::format("mesh[{}] '{}' has no vertex {} - skipping import", i, mesh->mName.C_Str(), !mesh->HasPositions() ? "positions" : "normals"));
          continue;
        }
        ctx.submesh_of_mesh[i] = next_submesh++;
      }
    }

    void process_mesh(const aiMesh* mesh, uint32_t mesh_idx, assimp_import_ctx& ctx, model_data& data) {
      OTHER_ASSERT(mesh != nullptr, "Mesh is null");

      const uint32_t submesh_idx = static_cast<uint32_t>(data.submeshes.size());
      OTHER_ASSERT(ctx.submesh_of_mesh[mesh_idx] == submesh_idx, "Mesh remap table out of sync at mesh {}", mesh_idx);

      submesh& sm = data.submeshes.emplace_back();
      sm.name = mesh->mName.C_Str();
      sm.sub_mesh_id = submesh_idx;
      sm.material_index = mesh->mMaterialIndex;
      sm.base_vertex = static_cast<uint32_t>(data.vertices.size());
      sm.vert_cnt = mesh->mNumVertices;
      sm.base_idx = static_cast<uint32_t>(data.indices.size() * 3);
      sm.local_transform = glm::mat4(1.f);  // overwritten with the owning node's transform during traversal

      CORE_LOG_DEBUG("Processing mesh[{}] '{}' -> submesh[{}]", mesh_idx, sm.name, submesh_idx);

      if (!mesh->HasTextureCoords(0)) {
        ctx.warn(std::format("mesh[{}] '{}' has no texture coordinates - defaulting to (0, 0)", mesh_idx, sm.name));
      }

      bounding_box& bounds = sm.bounds;
      bounds.min = glm::vec3(std::numeric_limits<float>::max());
      bounds.max = glm::vec3(std::numeric_limits<float>::lowest());
      for (uint32_t v = 0; v < mesh->mNumVertices; ++v) {
        vertex& vert = data.vertices.emplace_back();
        vert.position = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
        vert.normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };

        if (mesh->HasTangentsAndBitangents()) {
          vert.tangent = { mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z };
          vert.bitangent = { mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z };
        }

        if (mesh->HasTextureCoords(0)) {
          vert.tex_coord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
        }

        bounds.min = glm::min(bounds.min, vert.position);
        bounds.max = glm::max(bounds.max, vert.position);
      }

      uint32_t accepted_faces = 0;
      for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices != 3) {
          /// triangulation is requested from assimp, so a violating face is bad data, not a code bug
          ctx.warn(std::format("mesh[{}] '{}' face[{}] has {} indices (expected 3) - skipping face", mesh_idx, sm.name, f, face.mNumIndices));
          continue;
        }

        index& idx = data.indices.emplace_back();
        idx = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };
        ++accepted_faces;
      }
      sm.idx_cnt = accepted_faces * 3;
    }

    /// pass 2 - geometry, model bounds ; pass 4 - bone rigging (weights land in accepted-submesh vertex ranges)
    void process_assimp_meshes(assimp_import_ctx& ctx, model_data& data) {
      PROFILE_SECTION("model_importer::import_assimp--process-meshes");
      const aiScene* scene = ctx.scene;

      for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        if (ctx.submesh_of_mesh[i] == assimp_import_ctx::kSkipped) {
          continue;
        }
        process_mesh(scene->mMeshes[i], i, ctx, data);
      }

      data.bounds = bounding_box::empty;
      for (const submesh& sm : data.submeshes) {
        data.bounds.min = glm::min(data.bounds.min, sm.bounds.min);
        data.bounds.max = glm::max(data.bounds.max, sm.bounds.max);
      }

      data.skel.bone_influence.resize(data.vertices.size());
      for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
        const uint32_t sm_idx = ctx.submesh_of_mesh[m];
        if (sm_idx == assimp_import_ctx::kSkipped) {
          continue;
        }

        const aiMesh* mesh = scene->mMeshes[m];
        submesh& sm = data.submeshes[sm_idx];
        if (mesh->mNumBones == 0) {
          sm.rigged = false;
          continue;
        }

        sm.rigged = true;
        for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
          const aiBone* ai_bone = mesh->mBones[b];

          bool has_nonzero_weights = false;
          for (uint32_t w = 0; w < ai_bone->mNumWeights; ++w) {
            if (ai_bone->mWeights[w].mWeight > 0.000001f) {
              has_nonzero_weights = true;
              break;
            }
          }
          if (!has_nonzero_weights) {
            continue;
          }

          int32_t bone_idx = data.skel.get_bone_index(ai_bone->mName.C_Str());
          if (bone_idx == -1) {
            continue;
          }

          data.skel.bones[bone_idx].offset_matrix = mat4_from_ai_mat4(ai_bone->mOffsetMatrix);
          for (uint32_t w = 0; w < ai_bone->mNumWeights; ++w) {
            const aiVertexWeight& weight = ai_bone->mWeights[w];
            data.skel.bone_influence[sm.base_vertex + weight.mVertexId].add_bone_data(bone_idx, weight.mWeight);
          }
        }
      }
    }

    /// pass 3 - node hierarchy; assimp mesh references map through the remap table
    void traverse_assimp_nodes(const aiNode* node, uint32_t node_idx, assimp_import_ctx& ctx, model_data& data, uint32_t level) {
      OTHER_ASSERT(node != nullptr, "aiNode is null");

      {
        mesh_node& n = data.nodes[node_idx];
        n.name = node->mName.C_Str();
        n.local_transform = mat4_from_ai_mat4(node->mTransformation);
        n.inverse_local_transform = glm::inverse(n.local_transform);
        CORE_LOG_DEBUG("{}Node[{}] name: '{}', num_children: {}, num_meshes: {}", std::string(level * 2, ' '), node_idx, n.name, node->mNumChildren, node->mNumMeshes);

        for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
          const uint32_t sm_idx = ctx.submesh_of_mesh[node->mMeshes[i]];
          if (sm_idx == assimp_import_ctx::kSkipped) {
            continue;
          }

          submesh& sm = data.submeshes[sm_idx];
          sm.node_id = node_idx;
          sm.local_transform = n.local_transform;
          n.sub_meshes.push_back(sm_idx);
        }

        n.children.resize(node->mNumChildren);
      }

      /// n is invalidated by the emplace_back below - re-index data.nodes inside the loop
      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        data.nodes.emplace_back().parent = node_idx;
        const uint32_t child_idx = static_cast<uint32_t>(data.nodes.size() - 1);
        data.nodes[node_idx].children[i] = child_idx;
        traverse_assimp_nodes(node->mChildren[i], child_idx, ctx, data, level + 1);
      }
    }

    void process_assimp_nodes(assimp_import_ctx& ctx, model_data& data) {
      PROFILE_SECTION("model_importer::import_assimp--process-nodes");

      data.nodes.emplace_back();
      traverse_assimp_nodes(ctx.scene->mRootNode, 0, ctx, data, 0);

      /// bones created from animation channels rig their node's submeshes directly
      for (mesh_node& node : data.nodes) {
        int32_t bone_idx = data.skel.get_bone_index(node.name);
        if (bone_idx < 0) {
          continue;
        }

        data.skel.bones[bone_idx].offset_matrix = glm::mat4(1.f);

        for (uint32_t sm_idx : node.sub_meshes) {
          submesh& sm = data.submeshes[sm_idx];
          sm.rigged = true;

          CORE_LOG_DEBUG("Rigging submesh[{}] '{}' to node bone '{}'", sm_idx, sm.name, node.name);
          for (uint32_t v = 0; v < sm.vert_cnt; ++v) {
            for (uint32_t slot = 0; slot < bone_influence::kMaxBones; ++slot) {
              data.skel.bone_influence[sm.base_vertex + v].add_bone_data(bone_idx, 1.f);
            }
          }
        }
      }
    }

    std::string texture_path(const aiMaterial* ai_material, std::initializer_list<aiTextureType> candidates) {
      for (aiTextureType type : candidates) {
        if (ai_material->GetTextureCount(type) == 0) {
          continue;
        }

        aiString path;
        if (ai_material->GetTexture(type, 0, &path) != aiReturn_SUCCESS) {
          continue;
        }

        std::string result = path.C_Str();
        if (result.empty()) {
          continue;
        }

        /// '*N' references a texture embedded in the file (common in .glb); doc 02 decides how to consume these
        if (result.front() == '*') {
          return std::format("embedded:{}", result.substr(1));
        }
        return result;
      }

      return {};
    }

    /// pass 5 - import-side material handoff; texture paths stay relative to the model file's directory
    void process_assimp_materials(const aiScene* scene, model_data& data) {
      PROFILE_SECTION("model_importer::import_assimp--process-materials");

      data.materials.reserve(scene->mNumMaterials);
      for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* ai_material = scene->mMaterials[i];
        OTHER_ASSERT(ai_material != nullptr, "Material {} is null", i);

        imported_material& mat = data.materials.emplace_back();

        aiString mat_name;
        if (ai_material->Get(AI_MATKEY_NAME, mat_name) == aiReturn_SUCCESS) {
          mat.name = mat_name.C_Str();
        }

        aiColor4D base_color;
        if (ai_material->Get(AI_MATKEY_BASE_COLOR, base_color) == aiReturn_SUCCESS ||
            ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, base_color) == aiReturn_SUCCESS) {
          mat.base_color = { base_color.r, base_color.g, base_color.b, base_color.a };
        }

        aiColor3D emissive;
        if (ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == aiReturn_SUCCESS) {
          mat.emissive_color = { emissive.r, emissive.g, emissive.b };
        }

        if (ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness) != aiReturn_SUCCESS) {
          mat.roughness = 1.f;
        }
        if (ai_material->Get(AI_MATKEY_METALLIC_FACTOR, mat.metalness) != aiReturn_SUCCESS &&
            ai_material->Get(AI_MATKEY_REFLECTIVITY, mat.metalness) != aiReturn_SUCCESS) {
          mat.metalness = 0.f;
        }

        mat.base_color_texture = texture_path(ai_material, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
        mat.normal_texture = texture_path(ai_material, { aiTextureType_NORMALS, aiTextureType_HEIGHT });
        mat.metallic_roughness_texture = texture_path(ai_material, { aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_UNKNOWN });
        mat.emissive_texture = texture_path(ai_material, { aiTextureType_EMISSIVE });

        CORE_LOG_DEBUG(" - Material[{}] '{}': roughness: {}, metalness: {}, textures: [base: '{}', normal: '{}', metallic-roughness: '{}', emissive: '{}']",
                       i, mat.name, mat.roughness, mat.metalness, mat.base_color_texture, mat.normal_texture, mat.metallic_roughness_texture, mat.emissive_texture);
      }
    }

  }  // namespace

  model_import_result import(const filepath& file_path) {
    PROFILE_SECTION("model_importer::import");

    /// model files are data: every failure here is an error result, never an assert
    if (!std::filesystem::exists(file_path)) {
      return { .error = std::format("model file does not exist: {}", file_path.string()) };
    }

    const std::string extension = file_path.extension().string();
    if (extension == ".omdl") {
      return model_importer::import_omdl(file_path);
    }
    if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" ||
        extension == ".glb" || extension == ".dae" || extension == ".3ds") {
      return model_importer::import_assimp(file_path);
    }

    return { .error = std::format("unsupported model format '{}'", extension) };
  }

  model_data build(const std::string& name, std::span<const vertex> vertices, std::span<const index> indices) {
    PROFILE_SECTION("model_importer::build--from-vertices");

    model_data data;
    data.name = name;

    if (vertices.empty() || indices.empty()) {
      CORE_LOG_ERROR("Cannot build model '{}' from empty geometry", name);
      return data;
    }

    data.vertices = { vertices.begin(), vertices.end() };
    data.indices = { indices.begin(), indices.end() };

    submesh& sm = data.submeshes.emplace_back();
    sm.base_vertex = 0;
    sm.base_idx = 0;
    sm.vert_cnt = static_cast<uint32_t>(vertices.size());
    sm.idx_cnt = static_cast<uint32_t>(indices.size() * 3);
    sm.sub_mesh_id = 0;
    sm.local_transform = glm::mat4(1.f);
    sm.name = name;

    data.bounds = bounding_box::empty;
    for (const vertex& v : vertices) {
      data.bounds.min = glm::min(data.bounds.min, v.position);
      data.bounds.max = glm::max(data.bounds.max, v.position);
    }

    mesh_node& node = data.nodes.emplace_back();
    node.name = name;
    node.local_transform = glm::mat4(1.f);
    node.inverse_local_transform = glm::mat4(1.f);
    node.sub_meshes.push_back(0);

    return data;
  }

  namespace model_importer {

    model_import_result import_assimp(const filepath& file_path) {
      PROFILE_SECTION("model_importer::import_assimp");

      const uint32_t flags =
        aiProcess_CalcTangentSpace         // create binormals/tangents just in case
        | aiProcess_Triangulate            // make sure we're triangles
        | aiProcess_SortByPType            // split meshes by primitive type
        | aiProcess_GenNormals             // make sure we have legit normals
        | aiProcess_GenUVCoords            // convert UVs if required
        | aiProcess_OptimizeMeshes         // batch draws where possible
        | aiProcess_LimitBoneWeights       // if more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
        | aiProcess_ValidateDataStructure  // validation
        | aiProcess_GlobalScale            // e.g. convert cm to m for fbx import (and other formats where cm is native)
        ;

      Assimp::Importer importer;
      importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

      const aiScene* scene = nullptr;
      {
        PROFILE_SECTION("model_importer::import_assimp--read-file");
        scene = importer.ReadFile(file_path.string(), flags);
      }

      if (scene == nullptr) {
        return { .error = std::format("failed to import '{}' [{}]", file_path.string(), importer.GetErrorString()) };
      }

      if constexpr (kDebugAssimpImporting) {
        log_scene_metadata(scene);
      }

      if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) {
        return { .error = std::format("incomplete scene in '{}' [{}]", file_path.string(), importer.GetErrorString()) };
      }

      CORE_LOG_DEBUG("Model loaded: {} ({} meshes, {} materials, {} textures, {} animations)", file_path.string(),
                     scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures, scene->mNumAnimations);

      assimp_import_ctx ctx{ .scene = scene };

      model_data data;
      data.name = file_path.filename().stem().string();
      data.global_transform = mat4_from_ai_mat4(scene->mRootNode->mTransformation);
      data.inverse_global_transform = glm::inverse(data.global_transform);

      process_assimp_skeleton(scene, data.skel, ctx.warnings);
      map_accepted_meshes(ctx);
      process_assimp_meshes(ctx, data);
      process_assimp_nodes(ctx, data);
      process_assimp_materials(scene, data);
      /// animation clips are not extracted yet: model_data grows clip storage with doc 03

      for (bone_influence& infl : data.skel.bone_influence) {
        infl.normalize();
      }

      for (uint32_t v = 0; v < data.vertices.size(); ++v) {
        const bone_influence& infl = data.skel.bone_influence[v];
        vertex& vert = data.vertices[v];
        for (uint32_t b = 0; b < bone_influence::kMaxBones; ++b) {
          vert.bone_ids[b] = infl.bone_ids[b];
          vert.bone_weights[b] = infl.weights[b];
        }
      }

      if (!data.valid()) {
        return { .error = std::format("'{}' contains no importable geometry", file_path.string()), .warnings = std::move(ctx.warnings) };
      }

      return { .data = std::move(data), .warnings = std::move(ctx.warnings) };
    }

    model_import_result import_omdl(const filepath& file_path) {
      /// .omdl is specified (doc 01 section 7) but the reader lands with the asset-pack work
      return { .error = std::format("'{}': .omdl baked models are specified but not implemented yet (lands with asset packs); re-export as .gltf/.glb", file_path.string()) };
    }

  }  // namespace model_importer
}  // namespace other
