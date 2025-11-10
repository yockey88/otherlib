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
      case value_type::STRING: ss << unwrap_as<std::string>(); break;
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

  void value::aquire() {
    mutex.lock();
  }

  void value::release() {
    mutex.unlock();
  }

}  // namespace other