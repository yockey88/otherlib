/**
 * \file renderer/resource_tag.cpp
 **/
#include "renderer/resource_tag.hpp"

namespace other {

  resource_tag resource_tag::from(const std::string_view str) {
    OTHER_ASSERT(!str.empty(), "Resource tag can not be empty!");
    return resource_tag{ FNV(str) };
  }

}  // namespace other