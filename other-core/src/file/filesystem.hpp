/**
 * \file filesystem/file_system.hpp
 **/
#ifndef OTHER_CORE_FILE_FILE_SYSTEM_HPP
#define OTHER_CORE_FILE_FILE_SYSTEM_HPP

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "core/ref.hpp"
#include "core/subsystem.hpp"
#include "event/event_system.hpp"
#include "file/directory.hpp"
#include "file/file_handle.hpp"
#include "file/local_file.hpp"
#include "file/remote_file.hpp"
#include "file/virtual_file.hpp"

namespace other {

  struct resolved_path {
    std::string mount_name;
    std::vector<std::string> relative_path_components;
    std::string file_name;
    std::string extension;

    bool is_valid() const { return !mount_name.empty(); }
  };

  /**
   * \note this should only ever be used by the main thread
   * \todo make this thread safe, or add a request/response message
   **/
  class file_system : public subsystem<file_system> {
   public:
    file_system() = default;
    virtual ~file_system() = default;

    /// files pathed as mount_name://relative/path
    static constexpr std::string_view kPathSeparator = "://";
    static constexpr std::string_view kSystemSeparator =
#ifdef _WIN32
      ":/";
#else
      /// look for ~
      "/";
#endif

    static inline filepath get_cwd() {
      return std::filesystem::current_path().string();
    }

    void initialize_file_events(event_system& events);
    void initialize_directory_structure(const std::vector<std::string_view>& mounts = {});
    void shutdown_file_system();
    void poll_files();

    /// parses both system and engine paths into its components
    /// e.g. "assets://textures/grass.png" -> { "assets", "textures/grass.png", "grass.png" }
    ///  or  "C:/path/to/file.txt" -> { "C", "path/to/file.txt", "file.txt" }
    static resolved_path resolve_path(const std::string_view engine_path);

    ref<directory> mount_directory(const std::string_view mount_name, const filepath& path);
    ref<directory> mount_virtual(const std::string_view mount_name);
    void add_toplevel_file(ref<file_handle> file);

    void unmount(const std::string_view mount_name);

    bool is_mounted(const std::string_view mount_name) const;

    ref<file_handle> get_file(const std::string_view engine_path);

    std::vector<std::string> mounted_names() const;
    ref<directory> get_mount(const std::string_view mount_name) const;
    ref<directory> get_or_create_mount(const std::string_view mount_name, const filepath& path = "");
    resolved_path deep_search_for_mount(const filepath& path) const;

    bool path_exists(const std::string_view engine_path) const;
    bool file_exists(const std::string_view engine_path) const;
    bool directory_exists(const std::string_view engine_path) const;

    ref<file_handle> find_file(const std::string_view name, const std::string_view ext = "") const;
    ref<file_handle> open(const std::string_view engine_path) const;

    ref<local_file> register_local_file(const filepath& path);
    ref<virtual_file> create_asset_virtual_file(const std::string_view virtual_path);
    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view relative_path, std::vector<uint8_t>&& initial_data = {});
    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view file_name, const std::string_view ext, std::vector<uint8_t>&& initial_data = {});
    ref<remote_file> register_remote_file(const std::string_view mount_name, const std::string_view relative_path, const std::string_view url);

    task fetch_remote(ref<remote_file> file);
    void scan_directory(const std::string_view mount_name, bool recursive = false);

    const std::map<natural_t, ref<directory>>& get_all_mounts() const { return mounts; }
    const std::map<natural_t, ref<file_handle>>& get_all_files() const { return toplevel_files; }

   private:
    mutable std::mutex fs_mutex;

    event_system* events = nullptr;
    std::map<natural_t, ref<directory>> mounts;
    std::map<natural_t, ref<file_handle>> toplevel_files;

    ref<local_file> create_local_file(const filepath& path);

    ref<directory> walk_or_create_path(ref<directory> root, const std::vector<std::string>& components);
    ref<directory> walk_path(ref<directory> root, const std::vector<std::string>& components) const;
    void scan_directory_impl(ref<directory> dir, const filepath& disk_path, bool recursive);
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::resolved_path> : public formatter<std::string_view> {
    auto format(const other::resolved_path& path, format_context& ctx) const {
      std::string formatted = std::format("Mount: '{}', File Name: '{}'", path.mount_name, path.file_name);
      formatted += " [";

      if (!path.relative_path_components.empty()) {
        formatted += path.relative_path_components[0];
        for (size_t i = 1; i < path.relative_path_components.size(); ++i) {
          formatted += "/" + path.relative_path_components[i];
        }
      }

      formatted += "]";
      return formatter<std::string_view>::format(formatted, ctx);
    }
  };

}  // namespace std

OTHER_DEPENDENT_SUBSYSTEM(
  other::file_system,
  subsystem_profile::kArena,
  subsystem_profile::kLogger);

#endif  // OTHER_CORE_FILE_FILE_SYSTEM_HPP