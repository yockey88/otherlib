/**
 * \file core/page.cpp
 **/
#include "memory/page.hpp"

#include "core/logger.hpp"

namespace other {

  void* page::get_ptr_at(size_t offset) {
    OTHER_ASSERT(offset < kPageSize, "Offset out of bounds for page allocation.");
    return &storage[offset];
  }

}  // namespace other