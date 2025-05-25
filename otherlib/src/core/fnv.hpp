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

  constexpr uint64_t FNV(uint8_t* data, size_t length) {
    uint64_t hash = kFnvOffsetBasis;
    for (size_t i = 0; i < length; ++i) {
      hash ^= data[i];
      hash *= kFnvPrime;
    }
    hash ^= length;
    hash *= kFnvPrime;

    return hash;
  }

  constexpr uint64_t FNV(std::string_view str) {
    return FNV((uint8_t*)str.data(), str.length());
  }

}  // namespace other

#endif  // OTHER_CORE_FNV_HPP