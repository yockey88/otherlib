/**
 * \file fnv.hpp
 **/
#ifndef OTHER_CORE_FNV_HPP
#define OTHER_CORE_FNV_HPP

#include <cstdint>
#include <string_view>

namespace other {

  static constexpr uint64_t kFnvOffsetBasis = 0xBCF29CE484222325;
  static constexpr uint64_t kFnvPrime = 0x100000001B3;

  constexpr uint64_t FNV(std::string_view str) {
    uint64_t hash = kFnvOffsetBasis;
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