/**
 * \file file/file_watcher.cpp
 **/
#include "file/file_watcher.hpp"

#include <xxHash/xxh3.h>

#include "file/path_helpers.hpp"

namespace other {
  namespace {

    /// '/'-separated paths relative to @p root; the filter prunes excluded subtrees
    //  (bin/, obj/, .*/), which is the rebuild-loop safety mechanism — do not drop it.
    //  iteration tolerates transient races (files vanishing mid-scan) via error codes.
    void scan_subtree_impl(const filepath& root, const filepath& dir, const glob_set* filter, std::unordered_set<std::string>& out) {
      std::error_code ec;
      for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        const opt<std::string> rel = try_relative(it->path(), root);
        if (!rel.has_value()) {
          continue;
        }

        if (it->is_directory(ec)) {
          if (filter == nullptr || filter->may_contain(*rel)) {
            scan_subtree_impl(root, it->path(), filter, out);
          }
        } else if (it->is_regular_file(ec)) {
          if (filter == nullptr || filter->matches(*rel)) {
            out.insert(*rel);
          }
        }
      }
    }

  }  // namespace

  file_watcher::file_watcher(event_system& events, const filepath& path, watch_type type, watch_mode mode)
      : events(events), watch_path(path), type(type), mode(mode) {
    exists = std::filesystem::exists(watch_path);
    last_write_timestamp = exists ? std::filesystem::last_write_time(watch_path) : std::filesystem::file_time_type::min();
    if (exists && type == watch_type::FILE) {
      checksum = compute_checksum(watch_path);
    }
    if (exists && type == watch_type::DIRECTORY) {
      scan_subtree(subtree);
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

    if (type == watch_type::DIRECTORY) {
      poll_directory();
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

    std::unordered_set<std::string> current;
    scan_subtree(current);

    for (const std::string& rel : current) {
      if (!subtree.contains(rel)) {
        events.trigger_event("filesystem.watch-event",
                             file_event{ .type = file_event::type::CREATED, .path = watch_path / rel });
      }
    }
    for (const std::string& rel : subtree) {
      if (!current.contains(rel)) {
        events.trigger_event("filesystem.watch-event",
                             file_event{ .type = file_event::type::DELETED, .path = watch_path / rel });
      }
    }
    subtree = std::move(current);
  }

  void file_watcher::set_filter(const glob_set* set) {
    filter = set;

    /// rebaseline silently: paths tracked before the filter arrived (or under a wider
    //  previous filter) must not surface as DELETED on the next poll
    subtree.clear();
    if (type == watch_type::DIRECTORY && exists) {
      scan_subtree(subtree);
    }
  }

  void file_watcher::scan_subtree(std::unordered_set<std::string>& out) const {
    scan_subtree_impl(watch_path, watch_path, filter, out);
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
