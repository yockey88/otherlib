/**
 * \file serialization/serialization.cpp
 **/
#include "serialization/serialization.hpp"

namespace other {
  namespace serialization {

    void write_string_value(const std::string& str, std::vector<uint8_t>& out_bytes) {
      const uint8_t* str_bytes = reinterpret_cast<const uint8_t*>(str.data());
      out_bytes.append_range(std::span(str_bytes, str.size()));
    }

    void write_bytes(const void* data, size_t size, std::vector<uint8_t>& out_bytes) {
      const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
      out_bytes.append_range(std::span(bytes, size));
    }

    std::string read_string_value(const std::span<const uint8_t> buffer, uint64_t length, size_t& cursor) {
      OTHER_ASSERT(buffer.size() >= cursor + length, "Buffer too small to read string of length {}", length);
      std::span<const uint8_t> str_span = buffer.subspan(cursor, length);
      cursor += length;
      return std::string{ reinterpret_cast<const char*>(str_span.data()), length };
    }

  }  // namespace serialization
}  // namespace other