/**
 * \file filesystem/directory.hpp
 **/
#ifndef OTHER_CORE_FILE_DIRECTORY_HPP
#define OTHER_CORE_FILE_DIRECTORY_HPP

#include <map>
#include <string>
#include <vector>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"
#include "file/file_handle.hpp"

namespace other {

  /// represents a directory node in the engine's virtual filesystem
  /**
   * directories can contain both child directories and file handles.
   * a mounted directory maps a name to a real or virtual directory tree,
   * allowing path resolution through the `mount_name://path/to/file` scheme.
   **/
  struct directory : public ref_counted {
    directory() = default;

    directory(event_system& events, const std::string_view name, const filepath& path, file_type type = file_type::LOCAL);

    ~directory() override = default;

    std::string to_string() const;

    void poll();

    const std::string& name() const { return dir_name; }
    const filepath& absolute_path() const { return abs_path; }

    ref<directory> get_child_directory(const std::string_view name) const;
    ref<directory> add_child_directory(const std::string_view name, const filepath& path);
    ref<directory> get_or_add_child_directory(const std::string_view name, const filepath& path);
    bool has_child_directory(const std::string_view name) const;
    bool directory_exists(const std::string_view relative_path) const;

    ref<file_handle> get_file(const std::string_view name) const;
    ref<file_handle> add_file(ref<file_handle> file);
    void remove_file(const std::string_view name);
    void remove_file_by_path(const filepath& path);
    bool has_file(const std::string_view name) const;

    ref<file_handle> find_file_by_name(const std::string_view name, const std::string_view ext = "") const;

    std::vector<ref<directory>> child_directories() const;
    std::vector<ref<file_handle>> files() const;

    static std::vector<std::string> split_path(const std::string_view path);

    const std::map<natural_t, ref<directory>>& get_children() const { return children; }
    const std::map<natural_t, ref<file_handle>>& get_files() const { return file_handles; }

    template <typename OS>
    void print(OS& os, size_t indent_level = 0) const {
      os << std::string(indent_level * 2, ' ') << std::format("DIR[{} : {}] : {} ({:#0x})", type, dir_name, abs_path.string(), hash);
      for (const auto& [hash, child] : children) {
        child->print(os, indent_level + 1);
      }
      for (const auto& [hash, file] : file_handles) {
        file->print(os, indent_level + 1);
      }
    }

   private:
    natural_t hash = 0;
    file_type type = file_type::LOCAL;
    std::string dir_name;
    filepath abs_path;

    /// keyed by FNV hash of the name
    std::map<natural_t, ref<directory>> children;
    std::map<natural_t, ref<file_handle>> file_handles;

    scope<file_watcher> watcher = nullptr;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_DIRECTORY_HPP