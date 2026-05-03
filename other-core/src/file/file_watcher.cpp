/**
 * \file file/file_watcher.cpp
 **/
#include "file/file_watcher.hpp"

namespace other {

  file_watcher::file_watcher(event_system& events, const filepath& path, watch_type type, watch_mode mode)
      : events(events), watch_path(path), type(type), mode(mode) {
    exists = std::filesystem::exists(watch_path);
  }

  void file_watcher::poll() {
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
    if (std::filesystem::is_regular_file(watch_path) && last_write_time != last_write_timestamp) {
      file_event event{ .type = file_event::type::MODIFIED, .path = watch_path };
      events.trigger_event("filesystem.watch-event", event);
      last_write_timestamp = last_write_time;
    }
  }

  scope<file_watcher> file_watcher::make_file_watcher(event_system& events, const filepath& path) {
    return make_scope<file_watcher>(events, path, watch_type::FILE);
  }

  scope<file_watcher> file_watcher::make_directory_watcher(event_system& events, const filepath& path, watch_mode mode) {
    return make_scope<file_watcher>(events, path, watch_type::DIRECTORY, mode);
  }

}  // namespace other