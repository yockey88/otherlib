/**
 * \file file/remote_file.cpp
 **/
#include "file/remote_file.hpp"

#include <algorithm>
#include <cstring>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  bool remote_file::open(file_mode mode) {
    PROFILE_SECTION("remote_file::open");

    if (mode == file_mode::WRITE || mode == file_mode::READ_WRITE) {
      CORE_LOG_ERROR("Remote file '{}' does not support write mode", file_name);
      return false;
    }

    if (mode == file_mode::CLOSED) {
      CORE_LOG_ERROR("Cannot open remote file '{}' in CLOSED mode", file_name);
      return false;
    }

    if (fetch_state != remote_fetch_state::COMPLETE) {
      CORE_LOG_WARN("Remote file '{}' has not been fetched yet, data will be empty until fetch completes", file_name);
    }

    current_mode = mode;
    return true;
  }

  void remote_file::close() {
    current_mode = file_mode::CLOSED;
  }

  std::string remote_file::read_all_as_string() {
    PROFILE_SECTION("remote_file::read_all_as_string");

    if (fetch_state != remote_fetch_state::COMPLETE) {
      CORE_LOG_WARN("Attempting to read remote file '{}' before fetch is complete", file_name);
      return {};
    }

    return std::string(cached_data.begin(), cached_data.end());
  }

  std::vector<uint8_t> remote_file::read_all() {
    PROFILE_SECTION("remote_file::read_all");

    if (fetch_state != remote_fetch_state::COMPLETE) {
      CORE_LOG_WARN("Attempting to read remote file '{}' before fetch is complete", file_name);
      return {};
    }

    return cached_data;
  }

  uint64_t remote_file::read(std::span<uint8_t> buffer, uint64_t offset) {
    PROFILE_SECTION("remote_file::read");
    OTHER_ASSERT(
      current_mode == file_mode::READ,
      "Remote file '{}' is not open for reading", file_name
    );

    if (fetch_state != remote_fetch_state::COMPLETE) {
      CORE_LOG_WARN("Attempting to read remote file '{}' before fetch is complete", file_name);
      return 0;
    }

    if (offset >= cached_data.size()) {
      return 0;
    }

    uint64_t available = static_cast<uint64_t>(cached_data.size()) - offset;
    uint64_t to_read = std::min(static_cast<uint64_t>(buffer.size()), available);
    std::memcpy(buffer.data(), cached_data.data() + offset, static_cast<size_t>(to_read));
    return to_read;
  }

  uint64_t remote_file::write(std::span<const uint8_t> /* data */) {
    CORE_LOG_ERROR("Cannot write to remote file '{}'", file_name);
    return 0;
  }

  task remote_file::fetch() {
    CORE_LOG_DEBUG("Beginning fetch for remote file '{}' from url '{}'", file_name, remote_url);
    fetch_state = remote_fetch_state::FETCHING;

    /// =======================================================================
    /// STUB: implement actual network fetching here
    ///
    /// this coroutine should:
    ///   1. issue an HTTP request to `remote_url`
    ///   2. yield (co_await task::awaiter{}) while waiting for the response
    ///   3. on success, write the response body into `cached_data`
    ///      and set `fetch_state = remote_fetch_state::COMPLETE`
    ///   4. on failure, set `fetch_state = remote_fetch_state::FAILED`
    ///      and log the error
    ///
    /// example skeleton:
    ///   auto response = co_await http_get(remote_url);
    ///   if (response.ok()) {
    ///     cached_data = std::move(response.body);
    ///     fetch_state = remote_fetch_state::COMPLETE;
    ///   } else {
    ///     fetch_state = remote_fetch_state::FAILED;
    ///   }
    /// =======================================================================

    CORE_LOG_WARN("remote_file::fetch() is a stub, no data will be fetched for '{}'", file_name);
    fetch_state = remote_fetch_state::FAILED;

    co_return;
  }

}  // namespace other