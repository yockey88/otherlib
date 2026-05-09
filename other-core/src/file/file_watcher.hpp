/**
 * \file file/file_watcher.hpp
 **/
#ifndef OTHER_CORE_FILE_FILE_WATCHER_HPP
#define OTHER_CORE_FILE_FILE_WATCHER_HPP

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "event/event_system.hpp"

namespace other {

  struct file_event {
    enum class type {
      CREATED,
      MODIFIED,
      DELETED,
      RENAMED,
    } type;
    filepath path;
    // for RENAMED events
    opt<filepath> old_path;
  };

  class file_watcher {
   public:
    enum class watch_mode {
      NON_RECURSIVE,
      RECURSIVE,
    };
    enum class watch_type {
      FILE,
      DIRECTORY,
    };

    file_watcher(event_system& events, const filepath& path, watch_type type, watch_mode mode = watch_mode::NON_RECURSIVE);
    ~file_watcher() = default;

    void poll();
    event_system& get_event_system() { return events; }

    static scope<file_watcher> make_file_watcher(event_system& events, const filepath& path);
    static scope<file_watcher> make_directory_watcher(event_system& events, const filepath& path, watch_mode mode = watch_mode::NON_RECURSIVE);

   private:
    bool exists = false;
    event_system& events;

    std::filesystem::file_time_type last_write_timestamp;

    filepath watch_path;
    watch_type type;
    watch_mode mode;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_FILE_WATCHER_HPP