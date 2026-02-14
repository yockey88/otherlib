/**
 * \file file/filesystem.cpp
 **/
#include "file/filesystem.hpp"

#include <algorithm>
#include <filesystem>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  resolved_path file_system::resolve_path(const std::string_view engine_path) {
    resolved_path result;

    auto sep_pos = engine_path.find(kPathSeparator);
    if (sep_pos == std::string_view::npos) {
      /// no mount prefix, treat entire path as relative with empty mount
      result.relative_path = std::string(engine_path);
      auto components = directory::split_path(engine_path);
      if (!components.empty()) {
        result.file_name = components.back();
      }
      return result;
    }

    result.mount_name = std::string(engine_path.substr(0, sep_pos));
    result.relative_path = std::string(engine_path.substr(sep_pos + kPathSeparator.size()));

    auto components = directory::split_path(result.relative_path);
    if (!components.empty()) {
      result.file_name = components.back();
    }

    return result;
  }

  ref<directory> file_system::mount_directory(const std::string_view mount_name, const filepath& path) {
    PROFILE_SECTION("file_system::mount_directory");
    OTHER_ASSERT(!mount_name.empty(), "Mount name cannot be empty");

    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Cannot mount '{}': path '{}' does not exist", mount_name, path.string());
      return nullptr;
    }

    if (!std::filesystem::is_directory(path)) {
      CORE_LOG_ERROR("Cannot mount '{}': path '{}' is not a directory", mount_name, path.string());
      return nullptr;
    }

    natural_t hash = FNV(mount_name);

    std::lock_guard lock(fs_mutex);
    auto it = mounts.find(hash);
    if (it != mounts.end()) {
      CORE_LOG_WARN("Mount '{}' already exists, returning existing mount", mount_name);
      return it->second;
    }

    auto dir = make_ref<directory>(mount_name, std::filesystem::absolute(path));
    mounts.insert({ hash, dir });

    CORE_LOG_INFO("Mounted directory '{}' -> '{}'", mount_name, path.string());
    return dir;
  }

  ref<directory> file_system::mount_virtual(const std::string_view mount_name) {
    PROFILE_SECTION("file_system::mount_virtual");
    OTHER_ASSERT(!mount_name.empty(), "Virtual mount name cannot be empty");

    natural_t hash = FNV(mount_name);

    std::lock_guard lock(fs_mutex);
    auto it = mounts.find(hash);
    if (it != mounts.end()) {
      CORE_LOG_WARN("Mount '{}' already exists, returning existing mount", mount_name);
      return it->second;
    }

    /// virtual mounts have no disk path
    auto dir = make_ref<directory>(mount_name, filepath{});
    mounts.insert({ hash, dir });

    CORE_LOG_INFO("Mounted virtual directory '{}'", mount_name);
    return dir;
  }

  void file_system::unmount(const std::string_view mount_name) {
    PROFILE_SECTION("file_system::unmount");

    natural_t hash = FNV(mount_name);

    std::lock_guard lock(fs_mutex);
    auto it = mounts.find(hash);
    if (it == mounts.end()) {
      CORE_LOG_WARN("Cannot unmount '{}': not currently mounted", mount_name);
      return;
    }

    mounts.erase(it);
    CORE_LOG_INFO("Unmounted '{}'", mount_name);
  }

  ref<directory> file_system::get_mount(const std::string_view mount_name) const {
    natural_t hash = FNV(mount_name);

    std::lock_guard lock(fs_mutex);
    auto it = mounts.find(hash);
    if (it != mounts.end()) {
      return it->second;
    }
    return nullptr;
  }

  ref<directory> file_system::get_or_create_mount(const std::string_view mount_name, const filepath& path) {
    ref<directory> mount = get_mount(mount_name);
    if (mount != nullptr) {
      return mount;
    }

    if (path.empty()) {
      return mount_virtual(mount_name);
    } else {
      if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        CORE_LOG_ERROR("Cannot create mount '{}': path '{}' does not exist or is not a directory", mount_name, path.string());
        return nullptr;
      }
      return mount_directory(mount_name, path);
    }
  }

  bool file_system::is_mounted(const std::string_view mount_name) const {
    std::lock_guard lock(fs_mutex);
    return mounts.find(FNV(mount_name)) != mounts.end();
  }

  ref<file_handle> file_system::get_file(const std::string_view engine_path) const {
    PROFILE_SECTION("file_system::get_file");

    resolved_path rp = resolve_path(engine_path);
    if (!rp.is_valid()) {
      CORE_LOG_ERROR("Cannot get file '{}': invalid engine path (no mount prefix)", engine_path);
      return nullptr;
    }

    ref<directory> mount = get_mount(rp.mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot get file '{}': mount '{}' not found", engine_path, rp.mount_name);
      return nullptr;
    }

    return mount->get_file(rp.relative_path);
  }

  std::vector<std::string> file_system::mounted_names() const {
    std::lock_guard lock(fs_mutex);
    std::vector<std::string> names;
    names.reserve(mounts.size());
    for (const auto& [hash, dir] : mounts) {
      names.push_back(dir->name());
    }
    return names;
  }

  ref<file_handle> file_system::open(const std::string_view engine_path) const {
    PROFILE_SECTION("file_system::open");

    resolved_path rp = resolve_path(engine_path);
    if (!rp.is_valid()) {
      CORE_LOG_ERROR("Cannot open '{}': invalid engine path (no mount prefix)", engine_path);
      return nullptr;
    }

    ref<directory> mount = get_mount(rp.mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot open '{}': mount '{}' not found", engine_path, rp.mount_name);
      return nullptr;
    }

    auto components = directory::split_path(rp.relative_path);
    if (components.empty()) {
      CORE_LOG_ERROR("Cannot open '{}': empty relative path", engine_path);
      return nullptr;
    }

    /// the last component is the file name, everything before is directory path
    std::string file_name = components.back();
    components.pop_back();

    ref<directory> target_dir = mount;
    if (!components.empty()) {
      target_dir = walk_path(mount, components);
      if (target_dir == nullptr) {
        CORE_LOG_ERROR("Cannot open '{}': directory path not found", engine_path);
        return nullptr;
      }
    }

    ref<file_handle> file = target_dir->get_file(file_name);
    if (file == nullptr) {
      /// try to open it as a local file if the mount has a disk path
      if (!mount->absolute_path().empty()) {
        filepath disk_path = mount->absolute_path();
        for (const auto& comp : components) {
          disk_path /= comp;
        }
        disk_path /= file_name;

        if (std::filesystem::exists(disk_path) && std::filesystem::is_regular_file(disk_path)) {
          auto local = make_ref<local_file>(disk_path);
          local->parent = target_dir.raw_ptr();
          return local;
        }
      }

      CORE_LOG_ERROR("Cannot open '{}': file '{}' not found in mount '{}'", engine_path, file_name, rp.mount_name);
      return nullptr;
    }

    return file;
  }

  ref<file_handle> file_system::find_file(const std::string_view name, const std::string_view ext) const {
    PROFILE_SECTION("file_system::find_file");
    std::lock_guard lock(fs_mutex);

    for (const auto& [hash, mount] : mounts) {
      auto result = mount->find_file_by_name(name, ext);
      if (result != nullptr) {
        return result;
      }
    }

    return nullptr;
  }

  bool file_system::path_exists(const std::string_view engine_path) const {
    return file_exists(engine_path) || directory_exists(engine_path);
  }

  bool file_system::file_exists(const std::string_view engine_path) const {
    bool is_system_file = std::filesystem::exists(filepath(engine_path)) && std::filesystem::is_regular_file(filepath(engine_path));
    if (is_system_file) {
      return true;
    }

    resolved_path rp = resolve_path(engine_path);
    return false;
  }

  bool file_system::directory_exists(const std::string_view engine_path) const {
    for (const auto& [hash, mount] : mounts) {
      if (mount->directory_exists(engine_path)) {
        return true;
      }
    }
    return false;
  }

  ref<virtual_file> file_system::create_virtual_file(const std::string_view mount_name, const std::string_view relative_path, std::vector<uint8_t>&& initial_data) {
    PROFILE_SECTION("file_system::create_virtual_file");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot create virtual file: mount '{}' not found", mount_name);
      return nullptr;
    }

    auto components = directory::split_path(relative_path);
    if (components.empty()) {
      CORE_LOG_ERROR("Cannot create virtual file: empty relative path");
      return nullptr;
    }

    std::string file_name = components.back();
    components.pop_back();

    ref<directory> target_dir = mount;
    if (!components.empty()) {
      target_dir = walk_or_create_path(mount, components);
    }

    filepath fp(file_name);
    std::string name = fp.filename().string();
    std::string ext = fp.extension().string();

    auto vfile = make_ref<virtual_file>(name, ext, std::move(initial_data));
    vfile->parent = target_dir.raw_ptr();

    /// build the virtual path
    std::string vpath = std::string(mount_name) + std::string(kPathSeparator) + std::string(relative_path);

    target_dir->add_file(vfile);

    CORE_LOG_DEBUG("Created virtual file '{}' in mount '{}'", vpath, mount_name);
    return vfile;
  }

  ref<virtual_file> file_system::create_virtual_file(const std::string_view mount_name, const std::string_view file_name, const std::string_view ext, std::vector<uint8_t>&& initial_data) {
    PROFILE_SECTION("file_system::create_virtual_file");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot create virtual file: mount '{}' not found", mount_name);
      return nullptr;
    }

    std::string full_name = std::string(file_name) + std::string(ext);
    auto vfile = make_ref<virtual_file>(full_name, ext, std::move(initial_data));
    vfile->parent = mount.raw_ptr();

    mount->add_file(vfile);

    CORE_LOG_DEBUG("Created virtual file '{}{}' in mount root '{}'", file_name, ext, mount_name);
    return vfile;
  }

  ref<remote_file> file_system::register_remote_file(
    const std::string_view mount_name,
    const std::string_view relative_path,
    const std::string_view url
  ) {
    PROFILE_SECTION("file_system::register_remote_file");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot register remote file: mount '{}' not found", mount_name);
      return nullptr;
    }

    auto components = directory::split_path(relative_path);
    if (components.empty()) {
      CORE_LOG_ERROR("Cannot register remote file: empty relative path");
      return nullptr;
    }

    std::string file_name = components.back();
    components.pop_back();

    ref<directory> target_dir = mount;
    if (!components.empty()) {
      target_dir = walk_or_create_path(mount, components);
    }

    filepath fp(file_name);
    std::string name = fp.filename().string();
    std::string ext = fp.extension().string();

    auto rfile = make_ref<remote_file>(name, ext, url);
    rfile->parent = target_dir.raw_ptr();

    target_dir->add_file(rfile);

    CORE_LOG_DEBUG("Registered remote file '{}' -> '{}' in mount '{}'", file_name, url, mount_name);
    return rfile;
  }

  task file_system::fetch_remote(ref<remote_file> file) {
    OTHER_ASSERT(file != nullptr, "Cannot fetch null remote file");
    co_await file->fetch();
    co_return;
  }

  void file_system::scan_directory(const std::string_view mount_name, bool recursive) {
    PROFILE_SECTION("file_system::scan_directory");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot scan: mount '{}' not found", mount_name);
      return;
    }

    if (mount->absolute_path().empty()) {
      CORE_LOG_WARN("Cannot scan virtual mount '{}': no disk path", mount_name);
      return;
    }

    CORE_LOG_INFO("Scanning mount '{}' at '{}'", mount_name, mount->absolute_path().string());
    scan_directory_impl(mount, mount->absolute_path(), recursive);
  }

  ref<directory> file_system::walk_or_create_path(ref<directory> root, const std::vector<std::string>& components) {
    ref<directory> current = root;
    for (const auto& comp : components) {
      ref<directory> child = current->get_child_directory(comp);
      if (child == nullptr) {
        filepath child_path;
        if (!current->absolute_path().empty()) {
          child_path = current->absolute_path() / comp;
        }
        child = current->add_child_directory(comp, child_path);
      }
      current = child;
    }
    return current;
  }

  ref<directory> file_system::walk_path(ref<directory> root, const std::vector<std::string>& components) const {
    ref<directory> current = root;
    for (const auto& comp : components) {
      current = current->get_child_directory(comp);
      if (current == nullptr) {
        return nullptr;
      }
    }
    return current;
  }

  void file_system::scan_directory_impl(ref<directory> dir, const filepath& disk_path, bool recursive) {
    if (!std::filesystem::exists(disk_path) || !std::filesystem::is_directory(disk_path)) {
      return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(disk_path)) {
      if (entry.is_regular_file()) {
        auto local = make_ref<local_file>(entry.path());
        dir->add_file(local);
      } else if (entry.is_directory() && recursive) {
        std::string child_name = entry.path().filename().string();
        auto child_dir = dir->add_child_directory(child_name, entry.path());
        scan_directory_impl(child_dir, entry.path(), recursive);
      }
    }
  }

}  // namespace other