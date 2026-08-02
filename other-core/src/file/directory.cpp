/**
 * \file file/directory.cpp
 **/
#include "file/directory.hpp"

#include <filesystem>
#include <ranges>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "file/local_file.hpp"
#include "file/path_helpers.hpp"

namespace other {

  directory::directory(event_system& events, const std::string_view name, const filepath& path, file_type type, mount_scope scope)
      : events(events), hash(FNV(name)), type(type), dir_name(name), abs_path(path), scope(scope) {
    watcher = file_watcher::make_directory_watcher(events, abs_path.empty() ? dir_name : abs_path, file_watcher::watch_mode::RECURSIVE);
    if (type == file_type::VIRTUAL) {
      abs_path = name;
      return;
    }
  }

  std::string directory::to_string() const {
    return std::format(
      "Directory:\n - Name: {}\n - Absolute Path: {}\n - Type: {}\n - Num Children: {}\n - Num Files: {}",
      dir_name,
      abs_path.string(),
      type,
      children.size(),
      file_handles.size());
  }

  void directory::recursive_scan() {
    if (type == file_type::VIRTUAL) {
      return;
    }
    if (!std::filesystem::exists(abs_path) || !std::filesystem::is_directory(abs_path)) {
      CORE_LOG_ERROR("Cannot scan directory '{}': path '{}' does not exist or is not a directory", dir_name, abs_path.string());
      return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(abs_path)) {
      if (entry.is_regular_file()) {
        auto local = make_ref<local_file>(events, entry.path(), filepath{ abs_path / entry.path().filename() }.string());
        add_file(local);
      } else if (entry.is_directory()) {
        std::string child_name = entry.path().filename().string();
        auto child_dir = add_child_directory(child_name, entry.path());
        child_dir->recursive_scan();
      }
    }
  }

  void directory::poll() {
    PROFILE_SECTION("directory::poll");
    if (watcher) {
      watcher->poll();
    }

    for (auto& [hash, child] : children) {
      child->poll();
    }
    for (auto& [hash, file] : file_handles) {
      file->poll();
    }
  }

  ref<directory> directory::get_child_directory(const std::string_view name) const {
    natural_t hash = FNV(name);
    auto it = children.find(hash);
    if (it != children.end()) {
      return it->second;
    }

    return nullptr;
  }

  ref<directory> directory::add_child_directory(const std::string_view name, const filepath& path) {
    PROFILE_SECTION("directory::add_child_directory");

    natural_t hash = FNV(name);
    auto it = children.find(hash);
    if (it != children.end()) {
      CORE_LOG_WARN("Child directory '{}' already exists in '{}'", name, dir_name);
      return it->second;
    }

    CORE_LOG_DEBUG(" - adding child directory '{}' with path '{}' to '{}'", name, path.string(), dir_name);
    auto child = make_ref<directory>(events, name, path, path.empty() ? file_type::VIRTUAL : file_type::LOCAL, scope);
    children.insert({ hash, child });
    return child;
  }

  ref<directory> directory::get_or_add_child_directory(const std::string_view name, const filepath& path) {
    ref<directory> child = get_child_directory(name);
    if (child != nullptr) {
      return child;
    }
    return add_child_directory(name, path);
  }

  bool directory::has_child_directory(const std::string_view name) const {
    return children.find(FNV(name)) != children.end();
  }

  bool directory::directory_exists(const std::string_view relative_path) const {
    PROFILE_SECTION("directory::directory_exists");

    auto components = split_path(relative_path);
    const directory* current = this;

    for (const auto& comp : components) {
      ref<directory> child = current->get_child_directory(comp);
      if (child == nullptr) {
        return false;
      }
      /// we know this should stay in scope long enough
      current = child.raw_ptr();
    }

    return true;
  }

  bool directory::contains_path(const filepath& path) const {
    if (type == file_type::VIRTUAL) {
      return false;
    }
    return try_relative(std::filesystem::absolute(path), abs_path).has_value();
  }

  ref<file_handle> directory::get_file(natural_t hash) {
    auto it = file_handles.find(hash);
    if (it != file_handles.end()) {
      return it->second;
    }

    if (type == file_type::VIRTUAL) {
      return nullptr;
    }

    OTHER_ASSERT(std::filesystem::exists(abs_path) && std::filesystem::is_directory(abs_path), "Directory '{}' has invalid path '{}'", dir_name, abs_path.string());
    for (const auto& entry : std::filesystem::directory_iterator(abs_path)) {
      /// file_handle::hash() is FNV of the absolute path string, so hashing each candidate
      ///  the same way finds the on-disk file this hash would have been created from
      if (entry.is_regular_file() && FNV(entry.path().string()) == hash) {
        auto local = make_ref<local_file>(events, entry.path(), filepath{ abs_path / entry.path().filename() }.string());
        return add_file(local);
      }
    }

    return nullptr;
  }

  ref<file_handle> directory::get_file(const std::string_view name, const std::string_view ext) {
    for (const auto& [hash, file] : file_handles) {
      bool name_match = (file->name() == name);
      bool ext_match = ext.empty() || (file->extension() == ext);
      if (name_match && ext_match) {
        return file;
      }
    }

    if (type == file_type::VIRTUAL) {
      return nullptr;
    }

    OTHER_ASSERT(std::filesystem::exists(abs_path) && std::filesystem::is_directory(abs_path), "Directory '{}' has invalid path '{}'", dir_name, abs_path.string());
    for (const auto& entry : std::filesystem::directory_iterator(abs_path)) {
      if (entry.is_regular_file()) {
        std::string entry_name = entry.path().stem().string();
        std::string entry_ext = entry.path().extension().string();
        bool name_match = (entry_name == name);
        bool ext_match = ext.empty() || (entry_ext == ext);
        if (name_match && ext_match) {
          auto local = make_ref<local_file>(events, entry.path(), filepath{ abs_path / entry.path().filename() }.string());
          return add_file(local);
        }
      }
    }

    return nullptr;
  }

  ref<file_handle> directory::add_file(ref<file_handle> file) {
    PROFILE_SECTION("directory::add_file");
    OTHER_ASSERT(file != nullptr, "Cannot add null file handle to directory '{}'", dir_name);
    file->parent = this;

    natural_t hash = file->hash();
    if (auto it = file_handles.find(hash); it != file_handles.end()) {
      it->second = file;
    } else {
      file_handles.insert({ hash, file });
    }

    return file;
  }

  void directory::remove_file(natural_t hash) {
    auto it = file_handles.find(hash);
    if (it != file_handles.end()) {
      file_handles.erase(it);
    }
  }

  void directory::remove_file_by_path(const filepath& path) {
    for (auto it = file_handles.begin(); it != file_handles.end(); ++it) {
      if (it->second->absolute_path() == path) {
        file_handles.erase(it);
        return;
      }
    }
  }

  bool directory::has_file(natural_t hash) const {
    return file_handles.find(hash) != file_handles.end();
  }

  ref<file_handle> directory::find_file_by_name(const std::string_view name, const std::string_view ext) const {
    PROFILE_SECTION("directory::find_file_by_name");

    for (const auto& [hash, file] : file_handles) {
      bool name_match = (file->name() == name);
      bool ext_match = ext.empty() || (file->extension() == ext);
      if (name_match && ext_match) {
        return file;
      }
    }

    /// recurse into children
    for (const auto& [hash, child] : children) {
      auto result = child->find_file_by_name(name, ext);
      if (result != nullptr) {
        return result;
      }
    }

    return nullptr;
  }

  ostd::vector<ref<directory>> directory::child_directories() const {
    ostd::vector<ref<directory>> result;
    result.reserve(children.size());
    for (const auto& [hash, child] : children) {
      result.push_back(child);
    }
    return result;
  }

  ostd::vector<ref<file_handle>> directory::files() const {
    ostd::vector<ref<file_handle>> result;
    result.reserve(file_handles.size());
    for (const auto& [hash, file] : file_handles) {
      result.push_back(file);
    }
    return result;
  }

  ostd::vector<std::string> directory::split_path(const std::string_view path) {
    ostd::vector<std::string> components;
    std::string current;

    for (char c : path) {
      if (c == '/' || c == '\\') {
        if (!current.empty()) {
          components.push_back(std::move(current));
          current.clear();
        }
      } else {
        current += c;
      }
    }

    if (!current.empty()) {
      components.push_back(std::move(current));
    }

    return components;
  }

}  // namespace other