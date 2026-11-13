/**
 * \file renderer/util/pipeline_asset_validation.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP
#define OTHER_RENDERER_RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP

#include <toml++/toml.hpp>

#include "renderer/util/validation_result.hpp"

namespace other {
  namespace detail {

    validation_result validate_pipeline_toml(const toml::table& tbl);

  }  // namespace detail
}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_UTIL_PIPELINE_ASSET_VALIDATION_HPP