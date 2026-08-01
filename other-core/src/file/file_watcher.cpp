/**
 * \file file/file_watcher.cpp
 **/
#include "file/file_watcher.hpp"

#include <xxhash/xxh3.h>

namespace other {

  file_watcher::file_watcher(event_system& events, const filepath& path, watch_type type, watch_mode mode)
      : events(events), watch_path(path), type(type), mode(mode) {
    exists = std::filesystem::exists(watch_path);
    last_write_timestamp = exists ? std::filesystem::last_write_time(watch_path) : std::filesystem::file_time_type::min();
    if (exists) {
      checksum = compute_checksum(watch_path);
    }
  }

  void file_watcher::poll() {
    PROFILE_SECTION("file_watcher::poll");
    if (!exists) {
      // if (std::filesystem::exists(watch_path)) {
      //   // file_event event{ .type = file_event::type::RECREATED, .path = watch_path };
      //   // events.trigger_event("filesystem.watch-event", event);
      //   exists = true;
      // }
      return;
    }

    if (!std::filesystem::exists(watch_path)) {
      file_event event{ .type = file_event::type::DELETED, .path = watch_path };
      events.trigger_event("filesystem.watch-event", event);
      exists = false;
      return;
    }

    auto last_write_time = std::filesystem::last_write_time(watch_path);
    if (std::filesystem::is_regular_file(watch_path)) {
      auto time_since_last_write = last_write_time - last_write_timestamp;
      if (time_since_last_write < std::chrono::milliseconds(100)) {
        return;
      }
      last_write_timestamp = last_write_time;

      natural_t new_checksum = compute_checksum(watch_path);
      if (new_checksum != checksum) {
        file_event event{ .type = file_event::type::MODIFIED, .path = watch_path };
        events.trigger_event("filesystem.watch-event", event);
        checksum = new_checksum;
      }
    }
  }

  void file_watcher::poll_directory() {
    PROFILE_SECTION("file_watcher::poll_directory");

    // std::unordered_set<std::string> current;
    // scan_subtree(watch_path, current);

    // for (const std::string& rel : current) {
    //   if (!subtree.contains(rel)) {
    //     events.trigger_event("filesystem.watch-event",
    //                          file_event{ .type = file_event::type::CREATED, .path = watch_path / rel });
    //   }
    // }
    // for (const std::string& rel : subtree) {
    //   if (!current.contains(rel)) {
    //     events.trigger_event("filesystem.watch-event",
    //                          file_event{ .type = file_event::type::DELETED, .path = watch_path / rel });
    //   }
    // }
    // subtree = std::move(current);
  }

  scope<file_watcher> file_watcher::make_file_watcher(event_system& events, const filepath& path) {
    return make_scope<file_watcher>(events, path, watch_type::FILE);
  }

  scope<file_watcher> file_watcher::make_directory_watcher(event_system& events, const filepath& path, watch_mode mode) {
    return make_scope<file_watcher>(events, path, watch_type::DIRECTORY, mode);
  }

  natural_t file_watcher::compute_checksum(const filepath& path) {
    PROFILE_SECTION("file_watcher::compute_checksum");

    natural_t checksum = 0;
    if (std::filesystem::is_regular_file(path)) {
      std::ifstream file(path, std::ios::binary);
      if (file.is_open()) {
        std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        checksum = XXH3_64bits(data.data(), data.size());
      }
    }
    return checksum;
  }

}  // namespace other