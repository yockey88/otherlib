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

  /// path scheme used by the engine filesystem
  /**
   * the engine uses a URI-like paths scheme to unify access to local, virtual, and remote resources:
   *
   *   mount_name://relative/path/to/file
   *
   * examples:
   *   assets://textures/grass.png        - local file in the "assets" mount
   *   scenes://main_level.scene          - local file in the "scenes" mount
   *   memory://live_buffer.bin           - virtual file in the "memory" mount
   *   remote://cdn.example.com/model.fbx - remote file stub
   *
   * the file_system class resolves these paths, manages mounts, and provides
   * factory methods for creating virtual and remote file handles.
   **/

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

    /// the separator used in engine virtual paths
    static constexpr std::string_view kPathSeparator = "://";

    /// files pathed as `mount_name://relative/path`
    ref<directory> mount_directory(const std::string_view mount_name, const filepath& path);

    /// mounts a purely virtual directory (no disk backing)
    ref<directory> mount_virtual(const std::string_view mount_name);

    void unmount(const std::string_view mount_name);
    ref<directory> get_mount(const std::string_view mount_name) const;
    bool is_mounted(const std::string_view mount_name) const;

    std::vector<std::string> mounted_names() const;

    /// parses an engine path string into its components
    /// e.g. "assets://textures/grass.png" -> { "assets", "textures/grass.png", "grass.png" }
    static resolved_path resolve_path(const std::string_view engine_path);

    /// for remote files, this does NOT automatically start fetching
    ref<file_handle> open(const std::string_view engine_path) const;

    /// searches all mounts for a file with the given name and optional extension
    ref<file_handle> find_file(const std::string_view name, const std::string_view ext = "") const;

    bool file_exists(const std::string_view engine_path) const;

    /// creates a virtual file and registers it in the given mount
    /// \note mount must already exist (use mount_virtual to create one)
    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view relative_path, std::vector<uint8_t>&& initial_data = {});
    ref<virtual_file> create_virtual_file(const std::string_view mount_name, const std::string_view file_name, const std::string_view ext, std::vector<uint8_t>&& initial_data = {});

    /// registers a remote file in the given mount that will be lazily fetched
    ref<remote_file> register_remote_file(const std::string_view mount_name, const std::string_view relative_path, const std::string_view url);

    /// begins fetching a remote file, returns the task to be scheduled
    task fetch_remote(ref<remote_file> file);
    void scan_directory(const std::string_view mount_name, bool recursive = false);

   private:
    mutable std::mutex fs_mutex;

    /// mounted directories keyed by FNV hash of mount name
    std::map<natural_t, ref<directory>> mounts;

    ref<directory> walk_or_create_path(ref<directory> root, const std::vector<std::string>& components);
    ref<directory> walk_path(ref<directory> root, const std::vector<std::string>& components) const;
    void scan_directory_impl(ref<directory> dir, const filepath& disk_path, bool recursive);
  };

}  // namespace other

OTHER_SUBSYSTEM(other::file_system);

#endif  // OTHER_CORE_FILE_FILE_SYSTEM_HPP