/**
 * \file core/value.cpp
 **/
#include "core/value.hpp"

#include "core/logger.hpp"

#include "value.hpp"

namespace other {

  value::~value() {
    storage = nullptr;
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

  value value::create_opaque_handle(void* opaque_data) {
    value v = value();
    v.storage = make_ref<value_storage_impl<void*>>();
    v.storage->set_data(opaque_data);
    return v;
  }

  bool value::is_empty() const {
    return storage == nullptr ||
      storage->data() == nullptr ||
      storage->size() == 0;
  }

  void value::clear() {
    if (is_empty() || storage->val_type() == value_type::OPAQUE_HANDLE) {
      return;
    }
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
    return storage->val_type();
  }

  void value::aquire() {
    mutex.lock();
  }

  void value::release() {
    mutex.unlock();
  }

}  // namespace other