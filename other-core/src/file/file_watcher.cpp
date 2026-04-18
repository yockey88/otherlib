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
      return;
    }

    if (exists && !std::filesystem::exists(watch_path)) {
      file_event event{ .type = file_event::type::DELETED, .path = watch_path };
      events.trigger_event("filesystem.watch-event", event);
      exists = false;
      return;
    }
  }

  scope<file_watcher> file_watcher::make_file_watcher(event_system& events, const filepath& path) {
    return make_scope<file_watcher>(events, path, watch_type::FILE);
  }

  scope<file_watcher> file_watcher::make_directory_watcher(event_system& events, const filepath& path, watch_mode mode) {
    return make_scope<file_watcher>(events, path, watch_type::DIRECTORY, mode);
  }

}  // namespace other