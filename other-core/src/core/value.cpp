/**
 * \file core/value.cpp
 **/
#include "core/value.hpp"

#include "core/logger.hpp"

#include "value.hpp"

namespace other {

  value::value(void* data, size_t sz, value_type val_type) {
    if (val_type == value_type::OPAQUE_HANDLE) {
      auto storage_impl = make_ref<value_storage_impl<void*>>();
      storage_impl->overwrite_data(data, sizeof(void*));
      storage = storage_impl;
    } else {
      OTHER_ASSERT(sz == get_value_type_size(val_type), "Size mismatch when creating value: expected {}, got {}", get_value_type_size(val_type), sz);
      switch (val_type) {
        case value_type::INT8: storage = make_ref<value_storage_impl<int8_t>>(*reinterpret_cast<int8_t*>(data)); break;
        case value_type::INT16: storage = make_ref<value_storage_impl<int16_t>>(*reinterpret_cast<int16_t*>(data)); break;
        case value_type::INT32: storage = make_ref<value_storage_impl<int32_t>>(*reinterpret_cast<int32_t*>(data)); break;
        case value_type::INT64: storage = make_ref<value_storage_impl<int64_t>>(*reinterpret_cast<int64_t*>(data)); break;
        case value_type::UINT8: storage = make_ref<value_storage_impl<uint8_t>>(*reinterpret_cast<uint8_t*>(data)); break;
        case value_type::UINT16: storage = make_ref<value_storage_impl<uint16_t>>(*reinterpret_cast<uint16_t*>(data)); break;
        case value_type::UINT32: storage = make_ref<value_storage_impl<uint32_t>>(*reinterpret_cast<uint32_t*>(data)); break;
        case value_type::UINT64: storage = make_ref<value_storage_impl<uint64_t>>(*reinterpret_cast<uint64_t*>(data)); break;
        case value_type::FLOAT: storage = make_ref<value_storage_impl<float>>(*reinterpret_cast<float*>(data)); break;
        case value_type::DOUBLE: storage = make_ref<value_storage_impl<double>>(*reinterpret_cast<double*>(data)); break;
        case value_type::STRING:
          storage = make_ref<value_storage_impl<std::string>>(std::string(reinterpret_cast<const char*>(data), sz));
          break;
        default:
          OTHER_ASSERT(false, "Unsupported value type in value constructor from raw data");
      }
    }
  }

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
    v.storage->overwrite_data(opaque_data, sizeof(void*));
    return v;
  }

  std::string value::to_string() const {
    std::stringstream ss;

    if (is_empty()) {
      ss << "<empty>";
      return ss.str();
    }

    ss << std::format("<{}:", type());
    switch (type()) {
      case value_type::INT8: ss << unwrap_as<int8_t>(); break;
      case value_type::INT16: ss << unwrap_as<int16_t>(); break;
      case value_type::INT32: ss << unwrap_as<int32_t>(); break;
      case value_type::INT64: ss << unwrap_as<int64_t>(); break;
      case value_type::UINT8: ss << unwrap_as<uint8_t>(); break;
      case value_type::UINT16: ss << unwrap_as<uint16_t>(); break;
      case value_type::UINT32: ss << unwrap_as<uint32_t>(); break;
      case value_type::UINT64: ss << unwrap_as<uint64_t>(); break;
      case value_type::FLOAT: ss << unwrap_as<float>(); break;
      case value_type::DOUBLE: ss << unwrap_as<double>(); break;
      case value_type::OEBOOL: ss << (unwrap_as<bool>() ? "true" : "false"); break;
      case value_type::STRING: ss << as_string(); break;
      case value_type::OPAQUE_HANDLE: ss << std::format("{:p}", storage->data()); break;
      default:
        ss << "<unknown_type>";
        break;
    }
    ss << ">";

    return ss.str();
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

  void value::acquire() {
    mutex.lock();
  }

  void value::release() {
    mutex.unlock();
  }

}  // namespace other