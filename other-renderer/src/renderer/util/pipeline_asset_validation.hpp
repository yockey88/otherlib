/**
 * \file renderer/util/pipeline_asset_validation.hpp
 **/
#ifndef RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP
#define RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP

#include <toml++/toml.hpp>

#include "renderer/util/validation_result.hpp"

namespace other {
  namespace detail {

    validation_result validate_pipeline_toml(const toml::table& tbl);

  }  // namespace detail
}  // namespace other

#endif  // RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP