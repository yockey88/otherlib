/**
 * \file model/model_importer.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_IMPORTER_HPP
#define OTHER_RENDERER_MODEL_IMPORTER_HPP

#include <cstdint>
#include <stack>

#include "core/defines.hpp"

#include "model/model_data.hpp"

namespace other {

  struct model_import_result {
    opt<model_data> data = std::nullopt;
    std::string error;
    ostd::vector<std::string> warnings;
  };

  model_import_result import(const filepath& file_path);
  model_data build(const std::string& name, std::span<const vertex> vertices, std::span<const index> indices);

  namespace model_importer {

    model_import_result import_assimp(const filepath& file_path);
    model_import_result import_omdl(const filepath& file_path);

  }  // namespace model_importer
}  // namespace other

#endif  // OTHER_RENDERER_MODEL_IMPORTER_HPP