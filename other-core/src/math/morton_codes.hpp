/**
 * \file math/morton_codes.hpp
 **/
#ifndef OTHER_CORE_MATH_MORTON_CODES_HPP
#define OTHER_CORE_MATH_MORTON_CODES_HPP

#include <cstdint>

namespace other {

  uint64_t morton_code3d(uint32_t x, uint32_t y, uint32_t z);

  uint64_t morton_encode3d(uint32_t x, uint32_t y, uint32_t z);
  uint64_t magic_morton3d(uint32_t x, uint32_t y, uint32_t z);
  uint64_t lookup_morton_code3d(uint32_t x, uint32_t y, uint32_t z);

}  // namespace other

#endif  // OTHER_CORE_MATH_MORTON_CODES_HPP