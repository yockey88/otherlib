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
#include "file/directory.hpp"
#include "file/file_handle.hpp"
#include "file/local_file.hpp"
#include "file/remote_file.hpp"
#include "file/virtual_file.hpp"

namespace other {

  struct resolved_path {
    std::string mount_name;
    std::string relative_path;
    std::string file_name;

    bool is_valid() const { return !mount_name.empty(); }
  };

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

    ref<directory> mount_directory(const std::string_view mount_name, const filepath& path);
    ref<directory> mount_virtual(const std::string_view mount_name);

    void unmount(const std::string_view mount_name);

    bool is_mounted(const std::string_view mount_name) const;

    std::vector<std::string> mounted_names() const;
    ref<directory> get_mount(const std::string_view mount_name) const;

    /// parses both system and engine paths into its components
    /// e.g. "assets://textures/grass.png" -> { "assets", "textures/grass.png", "grass.png" }
    ///  or  "C:/path/to/file.txt" -> { "C", "path/to/file.txt", "file.txt" }
    static resolved_path resolve_path(const std::string_view engine_path);

    bool file_exists(const std::string_view engine_path) const;

    ref<file_handle> find_file(const std::string_view name, const std::string_view ext = "") const;
    ref<file_handle> open(const std::string_view engine_path) const;

    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view relative_path, std::vector<uint8_t>&& initial_data = {});
    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view file_name, const std::string_view ext, std::vector<uint8_t>&& initial_data = {});

    ref<remote_file> register_remote_file(const std::string_view mount_name, const std::string_view relative_path, const std::string_view url);

    task fetch_remote(ref<remote_file> file);
    void scan_directory(const std::string_view mount_name, bool recursive = false);

   private:
    mutable std::mutex fs_mutex;

    std::map<natural_t, ref<directory>> mounts;

    ref<directory> walk_or_create_path(ref<directory> root, const std::vector<std::string>& components);
    ref<directory> walk_path(ref<directory> root, const std::vector<std::string>& components) const;
    void scan_directory_impl(ref<directory> dir, const filepath& disk_path, bool recursive);
  };

}  // namespace other

OTHER_SUBSYSTEM(other::file_system);

#endif  // OTHER_CORE_FILE_FILE_SYSTEM_HPP