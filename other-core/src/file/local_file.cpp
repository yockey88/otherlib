/**
 * \file file/local_file.cpp
 **/
#include "file/local_file.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  bool local_file::open(file_mode mode) {
    PROFILE_SECTION("local_file::open");

    if (is_open()) {
      CORE_LOG_WARN("File '{}' is already open, closing before reopening", file_name);
      close();
    }

    std::ios_base::openmode flags = std::ios::binary;
    switch (mode) {
      case file_mode::READ:
        flags |= std::ios::in;
        break;
      case file_mode::WRITE:
        flags |= std::ios::out | std::ios::trunc;
        break;
      case file_mode::READ_WRITE:
        flags |= std::ios::in | std::ios::out;
        break;
      default:
        CORE_LOG_ERROR("Invalid file mode for file '{}'", file_name);
        return false;
    }

    stream.open(abs_path, flags);
    if (!stream.is_open()) {
      CORE_LOG_ERROR("Failed to open file '{}' at path '{}'", file_name, abs_path.string());
      return false;
    }

    current_mode = mode;
    return true;
  }

  void local_file::close() {
    if (stream.is_open()) {
      stream.close();
    }
    current_mode = file_mode::CLOSED;
  }

  std::string local_file::read_all_as_string() {
    PROFILE_SECTION("local_file::read_all_as_string");

    bool was_open = is_open();
    if (!was_open) {
      if (!open(file_mode::READ)) {
        return {};
      }
    }

    std::stringstream ss;
    ss << stream.rdbuf();

    if (!was_open) {
      close();
    }

    return ss.str();
  }

  std::vector<uint8_t> local_file::read_all() {
    PROFILE_SECTION("local_file::read_all");

    bool was_open = is_open();
    if (!was_open) {
      if (!open(file_mode::READ)) {
        return {};
      }
    }

    stream.seekg(0, std::ios::end);
    auto file_size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    if (file_size <= 0) {
      if (!was_open) {
        close();
      }
      return {};
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    stream.read(reinterpret_cast<char*>(buffer.data()), file_size);

    if (!was_open) {
      close();
    }

    return buffer;
  }

  uint64_t local_file::read(std::span<uint8_t> buffer, uint64_t offset) {
    PROFILE_SECTION("local_file::read");
    OTHER_ASSERT(is_open(), "Cannot read from closed file '{}'", file_name);
    OTHER_ASSERT(
      current_mode == file_mode::READ || current_mode == file_mode::READ_WRITE,
      "File '{}' is not open for reading", file_name
    );

    stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    return static_cast<uint64_t>(stream.gcount());
  }

  uint64_t local_file::write(std::span<const uint8_t> data) {
    PROFILE_SECTION("local_file::write");
    OTHER_ASSERT(is_open(), "Cannot write to closed file '{}'", file_name);
    OTHER_ASSERT(
      current_mode == file_mode::WRITE || current_mode == file_mode::READ_WRITE,
      "File '{}' is not open for writing", file_name
    );

    auto before = stream.tellp();
    stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    auto after = stream.tellp();
    return static_cast<uint64_t>(after - before);
  }

}  // namespace other