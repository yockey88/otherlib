/**
 * \file renderer/util/validation_result.hpp
 **/
#ifndef RENDERER_UTIL_VALIDATION_RESULT_HPP
#define RENDERER_UTIL_VALIDATION_RESULT_HPP

#include <string>

#include "core/defines.hpp"

namespace other {
  namespace detail {

    struct validation_result {
      bool valid = true;
      ostd::vector<std::string> errors;

      void fail(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
      }
    };

  }  // namespace detail
}  // namespace other

#endif  // RENDERER_UTIL_VALIDATION_RESULT_HPP
