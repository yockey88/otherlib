/**
 * \file kernel/value.cpp
 **/
#include "kernel/value.hpp"

namespace other {

  Value::Value() {
  }

  Value::~Value() {
    storage = nullptr;
  }

  Value Value::CreateOpaqueHandle(void* opaque_data) {
    Value v = Value();
    v.storage = NewRef<ValueStorageImpl<void*>>();
    v.storage->data = opaque_data;
    return v;
  }

  Value::Value(Value&& other) {
    storage = other.storage;
    other.storage = nullptr;
  }

  Value::Value(const Value& other) {
    storage = other.storage;
  }

  Value& Value::operator=(Value&& other) {
    storage = other.storage;
    other.storage = nullptr;
    return *this;
  }

  Value& Value::operator=(const Value& other) {
    storage = other.storage;
    return *this;
  }

  bool Value::IsEmpty() const {
    if (storage == nullptr) {
      return false;
    }

    bool empty = storage->data == nullptr;
    if (empty) {
      return true;
      // OE_ASSERT(storage->size == 0, "Size is not zero for empty value!");
      // OE_ASSERT(storage->value_type == ValueType::EMPTY_TYPE, "Value type is not EMPTY_TYPE for empty value!");
    }
    return empty;
  }

  void Value::Clear() {
    if (IsEmpty() || storage->value_type == ValueType::OPAQUE_HANDLE) {
      return;
    }
    // OE_ASSERT(storage != nullptr, "Storage is null!");
    storage = nullptr;
  }

  size_t Value::GetSize() const {
    if (storage == nullptr) {
      return 0;
    }
    return storage->size;
  }

  ValueType Value::GetType() const {
    if (storage == nullptr) {
      return ValueType::EMPTY_TYPE;
    }
    return storage->value_type;
  }

}  // namespace other