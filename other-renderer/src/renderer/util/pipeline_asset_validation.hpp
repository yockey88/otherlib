/**
 * \file renderer/util/pipeline_asset_validation.hpp
 **/
#ifndef RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP
#define RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP

#include <toml++/toml.hpp>

namespace other {
  namespace detail {

    struct validation_result {
      bool valid = true;
      std::vector<std::string> errors;

      void fail(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
      }
    };

    validation_result validate_pipeline_toml(const toml::table& tbl);

  }  // namespace detail
}  // namespace other

#endif  // RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP