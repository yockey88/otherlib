/**
 * \file fnv.hpp
 **/
#ifndef OTHER_CORE_FNV_HPP
#define OTHER_CORE_FNV_HPP

#include <string_view>

#include "core/defines.hpp"

namespace other {

  constexpr natural_t OTHER_API FNV(std::string_view str) {
    constexpr natural_t kFnvOffsetBasis = 0xBCF29CE484222325;
    constexpr natural_t kFnvPrime = 0x100000001B3;

    natural_t hash = kFnvOffsetBasis;
    for (size_t i = 0; i < str.size(); ++i) {
      hash ^= str[i];
      hash *= kFnvPrime;
    }
    hash ^= str.size();
    hash *= kFnvPrime;

    return hash;
  }

}  // namespace other

#endif  // OTHER_CORE_FNV_HPP