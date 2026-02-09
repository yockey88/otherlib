/**
 * \file file/virtual_file.cpp
 **/
#include "file/virtual_file.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  bool virtual_file::open(file_mode mode) {
    PROFILE_SECTION("virtual_file::open");

    if (mode == file_mode::CLOSED) {
      CORE_LOG_ERROR("Cannot open virtual file '{}' in CLOSED mode", file_name);
      return false;
    }

    current_mode = mode;
    cursor = 0;
    return true;
  }

  void virtual_file::close() {
    current_mode = file_mode::CLOSED;
    cursor = 0;
  }

  std::vector<uint8_t> virtual_file::read_all() {
    PROFILE_SECTION("virtual_file::read_all");
    return buffer;
  }

  uint64_t virtual_file::read(std::span<uint8_t> out, uint64_t offset) {
    PROFILE_SECTION("virtual_file::read");
    OTHER_ASSERT(
      current_mode == file_mode::READ || current_mode == file_mode::READ_WRITE,
      "Virtual file '{}' is not open for reading", file_name
    );

    uint64_t read_start = offset;
    if (read_start >= buffer.size()) {
      return 0;
    }

    uint64_t available = static_cast<uint64_t>(buffer.size()) - read_start;
    uint64_t to_read = std::min(static_cast<uint64_t>(out.size()), available);

    std::memcpy(out.data(), buffer.data() + read_start, static_cast<size_t>(to_read));
    cursor = read_start + to_read;
    return to_read;
  }

  uint64_t virtual_file::write(std::span<const uint8_t> data) {
    PROFILE_SECTION("virtual_file::write");
    OTHER_ASSERT(
      current_mode == file_mode::WRITE || current_mode == file_mode::READ_WRITE,
      "Virtual file '{}' is not open for writing", file_name
    );

    uint64_t write_end = cursor + data.size();
    if (write_end > buffer.size()) {
      buffer.resize(static_cast<size_t>(write_end));
    }

    std::memcpy(buffer.data() + cursor, data.data(), data.size());
    cursor = write_end;
    return static_cast<uint64_t>(data.size());
  }
}  // namespace other