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

    directory(const std::string_view name, const filepath& path)
        : dir_name(name), abs_path(path) {}

    ~directory() override = default;

    const std::string& name() const { return dir_name; }
    const filepath& absolute_path() const { return abs_path; }

    /// child directory access
    ref<directory> get_child_directory(const std::string_view name) const;
    ref<directory> add_child_directory(const std::string_view name, const filepath& path);
    bool has_child_directory(const std::string_view name) const;

    /// file access
    ref<file_handle> get_file(const std::string_view name) const;
    ref<file_handle> add_file(ref<file_handle> file);
    bool has_file(const std::string_view name) const;

    /// find a file by name with optional extension filter
    ref<file_handle> find_file_by_name(const std::string_view name, const std::string_view ext = "") const;

    /// list all immediate children
    std::vector<ref<directory>> child_directories() const;
    std::vector<ref<file_handle>> files() const;

    /// utility to split a path string into its components
    static std::vector<std::string> split_path(const std::string_view path);

   private:
    std::string dir_name;
    filepath abs_path;

    /// keyed by FNV hash of the name
    std::map<natural_t, ref<directory>> children;
    std::map<natural_t, ref<file_handle>> file_handles;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_DIRECTORY_HPP