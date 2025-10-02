/**
 * \file serialization/serialization.cpp
 **/
#include "serialization/serialization.hpp"

#include <fstream>
#include <string>

namespace other {
  namespace serialization {

    std::vector<uint8_t> read_file_to_bytes(const filepath& file_path) {
      std::ifstream file(file_path, std::ios::binary);
      OTHER_ASSERT(file.is_open(), "Failed to open project file");

      std::vector<uint8_t> buffer = {};

      size_t num_bytes = 0;
      file.seekg(0, std::ios::end);
      num_bytes = static_cast<size_t>(file.tellg());
      file.seekg(0, std::ios::beg);
      OTHER_ASSERT(num_bytes > 0, "Project file is empty");
      CORE_LOG_DEBUG("Reading file of size: {} bytes", num_bytes);

      buffer.resize(num_bytes);
      file.read(reinterpret_cast<char*>(buffer.data()), num_bytes);

      return buffer;
    }

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

    std::vector<uint8_t> read_bytes(const std::span<const uint8_t> buffer, uint64_t length, size_t& cursor) {
      OTHER_ASSERT(buffer.size() >= cursor + length, "Buffer to small to read {} bytes", length);
      auto sp = buffer.subspan(cursor, length);
      cursor += length;
      return { sp.begin(), sp.end() };
    }

  }  // namespace serialization
}  // namespace other