/**
 * \file core/value.cpp
 **/
#include "core/value.hpp"

#include "value.hpp"

namespace other {

  value::~value() {
    storage = nullptr;
  }

  value value::create_opaque_handle(void* opaque_data) {
    value v = value();
    v.storage = nullptr;  // NewRef<ValueStorageImpl<void*>>();
    v.storage->data() = opaque_data;
    return v;
  }

  value::value(value&& other) {
    storage = other.storage;
    other.storage = nullptr;
  }

  value::value(const value& other) {
    storage = other.storage;
  }

  value& value::operator=(value&& other) {
    storage = other.storage;
    other.storage = nullptr;
    return *this;
  }

  value& value::operator=(const value& other) {
    storage = other.storage;
    return *this;
  }

  bool value::is_empty() const {
    if (storage == nullptr) {
      return false;
    }

    bool empty = storage->data() == nullptr;
    if (empty) {
      return true;
      // OE_ASSERT(storage->size == 0, "Size is not zero for empty value!");
      // OE_ASSERT(storage->value_type == ValueType::EMPTY_TYPE, "Value type is not EMPTY_TYPE for empty value!");
    }
    return empty;
  }

  void value::clear() {
    if (is_empty() || storage->type == value_type::OPAQUE_HANDLE) {
      return;
    }
    // OE_ASSERT(storage != nullptr, "Storage is null!");
    storage = nullptr;
  }

  size_t value::size() const {
    if (storage == nullptr) {
      return 0;
    }
    return storage->size();
  }

  value_type value::type() const {
    if (storage == nullptr) {
      return value_type::EMPTY_TYPE;
    }
    return storage->type;
  }

  void value::aquire() {
    mutex.lock();
  }

  void value::release() {
    mutex.unlock();
  }

}  // namespace other