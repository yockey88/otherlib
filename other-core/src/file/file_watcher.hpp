/**
 * \file file/file_watcher.hpp
 **/
#ifndef OTHER_CORE_FILE_FILE_WATCHER_HPP
#define OTHER_CORE_FILE_FILE_WATCHER_HPP

#include <unordered_map>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "file/glob.hpp"

namespace other {

  class event_system;

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
    void poll_directory();

    event_system& get_event_system() { return events; }
    /// swaps the exclude filter and silently rebaselines the subtree snapshot
    void set_filter(const glob_set* set);

    static scope<file_watcher> make_file_watcher(event_system& events, const filepath& path);
    static scope<file_watcher> make_directory_watcher(event_system& events, const filepath& path, watch_mode mode = watch_mode::NON_RECURSIVE);

   private:
    struct file_time {
      filepath path;
      std::filesystem::file_time_type last_write_time;
    };

    friend void scan_subtree_impl(const filepath& root, const filepath& dir, const glob_set* filter, ostd::vector<file_time>& out);

    bool exists = false;
    event_system& events;
    // counts all files recursively under the watched directory;
    // used to detect new files in a directory that is not being watched recursively
    size_t directory_contained_files = 0;

    std::filesystem::file_time_type last_write_timestamp;

    filepath watch_path;
    watch_type type;
    watch_mode mode;

    /// relative path -> last write time; the per-poll diff is CREATED/DELETED from the
    ///  key set and MODIFIED from a changed timestamp
    ostd::vector<file_time> subtree;
    const glob_set* filter = nullptr;

    natural_t checksum = 0;

    size_t calculate_directory_file_count() const;

    natural_t compute_checksum(const filepath& path);
    void scan_subtree(ostd::vector<file_time>& out) const;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_FILE_WATCHER_HPP