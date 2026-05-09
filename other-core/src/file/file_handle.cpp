/**
 * \file file/file_handle.cpp
 **/
#include "file/file_handle.hpp"

namespace other {

  void file_handle::poll() {
    if (watcher) {
      watcher->poll();
    }
  }

  file_handle::file_handle(event_system& events, const std::string_view name, const std::string_view ext, const filepath& abs_path, const std::string_view virtual_path, file_type type)
      : file_name(name), file_extension(ext), abs_path(abs_path), virt_path(virtual_path), handle_type(type) {
    if (type == file_type::LOCAL) {
      watcher = file_watcher::make_file_watcher(events, abs_path);
    }
  }

}  // namespace other