/**
 * \file file/directory.cpp
 **/
#include "file/directory.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

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

    auto child = make_ref<directory>(name, path);
    children.insert({ hash, child });
    return child;
  }

  bool directory::has_child_directory(const std::string_view name) const {
    return children.find(FNV(name)) != children.end();
  }

  ref<file_handle> directory::get_file(const std::string_view name) const {
    natural_t hash = FNV(name);
    auto it = file_handles.find(hash);
    if (it != file_handles.end()) {
      return it->second;
    }
    return nullptr;
  }

  ref<file_handle> directory::add_file(ref<file_handle> file) {
    PROFILE_SECTION("directory::add_file");
    OTHER_ASSERT(file != nullptr, "Cannot add null file handle to directory '{}'", dir_name);

    natural_t hash = FNV(file->name());
    auto it = file_handles.find(hash);
    if (it != file_handles.end()) {
      CORE_LOG_WARN("File '{}' already exists in directory '{}'", file->name(), dir_name);
      return it->second;
    }

    file->parent = this;
    file_handles.insert({ hash, file });
    return file;
  }

  bool directory::has_file(const std::string_view name) const {
    return file_handles.find(FNV(name)) != file_handles.end();
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

  std::vector<ref<directory>> directory::child_directories() const {
    std::vector<ref<directory>> result;
    result.reserve(children.size());
    for (const auto& [hash, child] : children) {
      result.push_back(child);
    }
    return result;
  }

  std::vector<ref<file_handle>> directory::files() const {
    std::vector<ref<file_handle>> result;
    result.reserve(file_handles.size());
    for (const auto& [hash, file] : file_handles) {
      result.push_back(file);
    }
    return result;
  }

  std::vector<std::string> directory::split_path(const std::string_view path) {
    std::vector<std::string> components;
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