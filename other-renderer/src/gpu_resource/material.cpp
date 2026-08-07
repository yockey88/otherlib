/**
 * \file gpu_resource/material.cpp
 **/
#include "gpu_resource/material.hpp"

#include <filesystem>
#include <format>

#include <toml++/toml.hpp>

#include "core/fnv.hpp"
#include "core/profiler.hpp"

namespace other {

  namespace {

    opt<material_value> value_from_node(const toml::node& node) {
      if (node.is_floating_point()) {
        return material_value::from(static_cast<float>(node.as_floating_point()->get()));
      }
      if (node.is_boolean()) {
        return material_value::from(node.as_boolean()->get());
      }
      if (node.is_integer()) {
        return material_value::from(static_cast<int32_t>(node.as_integer()->get()));
      }
      if (const toml::array* arr = node.as_array(); arr != nullptr && arr->size() >= 2 && arr->size() <= 4) {
        glm::vec4 v(0.f);
        for (size_t i = 0; i < arr->size(); ++i) {
          const toml::node& e = (*arr)[i];
          if (e.is_floating_point()) {
            v[static_cast<glm::length_t>(i)] = static_cast<float>(e.as_floating_point()->get());
          } else if (e.is_integer()) {
            v[static_cast<glm::length_t>(i)] = static_cast<float>(e.as_integer()->get());
          } else {
            return std::nullopt;
          }
        }
        switch (arr->size()) {
          case 2: return material_value::from(glm::vec2(v));
          case 3: return material_value::from(glm::vec3(v));
          default: return material_value::from(v);
        }
      }
      return std::nullopt;
    }

  }  // namespace

  material_parse_result parse_material_toml(const filepath& path) {
    PROFILE_SECTION("parse_material_toml");
    material_parse_result result;

    toml::table table;
    try {
      table = toml::parse_file(path.string());
    } catch (const std::exception& e) {
      result.error = std::format("failed to parse material '{}': {}", path.string(), e.what());
      return result;
    } catch (...) {
      result.error = std::format("failed to parse material '{}': unknown error", path.string());
      return result;
    }

    material mat;
    if (const auto name = table.at_path("name"); name && name.is_string()) {
      mat.name = name.as_string()->get();
    } else {
      mat.name = path.stem().string();
    }

    if (const auto params = table.at_path("params"); params) {
      const toml::table* param_table = params.as_table();
      if (param_table == nullptr) {
        result.error = std::format("material '{}': [params] is not a table", path.string());
        return result;
      }
      for (const auto& [key, node] : *param_table) {
        const opt<material_value> value = value_from_node(node);
        if (!value.has_value()) {
          result.warnings.push_back(std::format("material '{}': param '{}' has an unsupported value shape (expected float, int, bool, or [f,f]/[f,f,f]/[f,f,f,f]); ignored",
                                                path.string(), key.str()));
          continue;
        }
        const natural_t hash = FNV(key.str());
        mat.params[hash] = *value;
        mat.param_names[hash] = std::string{ key.str() };
      }
    }

    if (const auto textures = table.at_path("textures"); textures) {
      const toml::table* texture_table = textures.as_table();
      if (texture_table == nullptr) {
        result.error = std::format("material '{}': [textures] is not a table", path.string());
        return result;
      }
      for (const auto& [key, node] : *texture_table) {
        if (!node.is_string()) {
          result.warnings.push_back(std::format("material '{}': texture slot '{}' is not a path string; ignored", path.string(), key.str()));
          continue;
        }
        mat.texture_paths[FNV(key.str())] = node.as_string()->get();
      }
    }

    result.mat = std::move(mat);
    return result;
  }

}  // namespace other
