/**
 * \file core/value_storage.cpp
 **/
#include "core/value_storage.hpp"

#include "core/logger.hpp"

namespace other {

  void* value_storage::unwrap_opaque_handle() {
    OTHER_ASSERT(val_type() == value_type::OPAQUE_HANDLE, "Value type is not OPAQUE_HANDLE!");
    return data();
  }

  std::string value_storage::unchecked_string_unwrap() const {
    const char* str_data = reinterpret_cast<const char*>(memory());
    OTHER_ASSERT(str_data != nullptr, "String data pointer is null!");
    return std::string(str_data, size());
  }

}  // namespace other