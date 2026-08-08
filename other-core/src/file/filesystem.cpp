/**
 * \file file/filesystem.cpp
 **/
#include "file/filesystem.hpp"

#include <filesystem>
#include <ranges>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "event/event_system.hpp"
#include "file/directory.hpp"
#include "file/path_helpers.hpp"

namespace other {
  namespace {

    static void expand_walk(const filepath& root_abs, const filepath& dir,
                            const glob_set& set, ostd::vector<resolved_file>& out) {
      PROFILE_SECTION("expand_walk");
      for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const opt<std::string> rel = try_relative(entry.path(), root_abs);
        OTHER_ASSERT(rel.has_value(), "walk escaped its root: '{}'", entry.path().string());

        if (entry.is_directory()) {
          if (set.may_contain(*rel)) {  /// prunes bin/, obj/, .git/ ...
            expand_walk(root_abs, entry.path(), set, out);
          }
        } else if (entry.is_regular_file() && set.matches(*rel)) {
          out.push_back({ entry.path(), virtualize(entry.path()) });
        }
      }
    }

  }  // namespace

  ostd::vector<resolved_file> file_system::expand(const filepath& root_abs, const glob_set& set) {
    PROFILE_SECTION("file_system::expand");
    OTHER_ASSERT(std::filesystem::is_directory(root_abs),
                 "expand root is not a directory: '{}'", root_abs.string());

    ostd::vector<resolved_file> out;
    expand_walk(root_abs, root_abs, set, out);

    std::ranges::sort(out, {}, &resolved_file::virtual_path);

    /// case-collision landmine detection: legal on Linux, breaks on NTFS
    for (size_t i = 1; i < out.size(); ++i) {
      if (detail::equals_case_insensitive(out[i - 1].virtual_path, out[i].virtual_path)) {
        CORE_LOG_WARN("case-colliding paths '{}' / '{}' — unportable",
                      out[i - 1].virtual_path, out[i].virtual_path);
      }
    }
    return out;
  }

  void file_system::initialize_file_events(event_system& events) {
    events.register_event("filesystem.watch-event");

    this->events = &events;
  }

  void file_system::initialize_directory_structure(const std::span<const std::string_view> mounts) {
    PROFILE_SECTION("file_system::initialize_directory_structure");
    for (const auto& mount : mounts) {
      if (!mount.empty()) {
        mount_virtual(mount);
      }
    }

    {
      PROFILE_SECTION("file_system::initialize_directory_structure - mount current working directory");
      ref<directory> cwd = mount_directory("cwd", std::filesystem::current_path());
      OTHER_ASSERT(cwd != nullptr, "Failed to mount current working directory");
    }
  }

  void file_system::shutdown_file_system() {
    /// mounts hold the event system by reference; dropping them here keeps a
    ///  re-initialized environment (tests, repeat runs) from firing watch events into a dead one
    std::lock_guard lock(fs_mutex);
    mounts.clear();
    toplevel_files.clear();
    events = nullptr;
  }

  void file_system::poll_files() {
    PROFILE_SECTION("file_system::poll_files");
    for (auto& [hash, mount] : mounts) {
      PROFILE_SECTION("file_system::poll_files--poll_mount");
      mount->poll();
    }
  }

  resolved_path file_system::resolve_path(const std::string_view engine_path) {
    PROFILE_SECTION("file_system::resolve_path");
    resolved_path result;

    std::string rel_path = {};
    auto sep_pos = engine_path.find(kPathSeparator);
    if (sep_pos == std::string_view::npos) {
      result.mount_name = "";
    } else {
      result.mount_name = std::string(engine_path.substr(0, sep_pos));
      rel_path = std::string(engine_path.substr(sep_pos + kPathSeparator.size()));
    }

    result.relative_path_components = directory::split_path(rel_path);
    if (!result.relative_path_components.empty()) {
      result.file_name = filepath{ result.relative_path_components.back() }.filename().stem().string();
      result.extension = filepath{ result.relative_path_components.back() }.extension().string();
      /// remove filename from relative path components to get directory path components
      result.relative_path_components.pop_back();
    }

    return result;
  }

  ref<directory> file_system::mount_directory(const std::string_view mount_name, const filepath& path, mount_scope scope) {
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

    OTHER_ASSERT(events != nullptr, "File system events not initialized when mounting '{}'", mount_name);
    const bool watch_subtree = scope == mount_scope::PROJECT;
    auto dir = make_ref<directory>(*events, mount_name, std::filesystem::absolute(path), file_type::LOCAL, scope, watch_subtree);
    mounts.insert({ hash, dir });

    CORE_LOG_DEBUG("Mounted directory '{}' -> '{}'", mount_name, path.string());
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

    OTHER_ASSERT(events != nullptr, "File system events not initialized when mounting '{}'", mount_name);
    /// virtual mounts have no disk path
    auto dir = make_ref<directory>(*events, mount_name, filepath{}, file_type::VIRTUAL);
    mounts.insert({ hash, dir });

    CORE_LOG_DEBUG("Mounted virtual directory '{}'", mount_name);
    return dir;
  }

  void file_system::add_toplevel_file(ref<file_handle> file) {
    PROFILE_SECTION("file_system::add_toplevel_file");
    OTHER_ASSERT(file != nullptr, "Cannot add null file handle to filesystem");

    CORE_LOG_DEBUG("Adding toplevel file to filesystem : {}", file->to_string());
    std::lock_guard lock(fs_mutex);
    toplevel_files.insert({ FNV(file->name()), file });
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
    CORE_LOG_DEBUG("Unmounted '{}'", mount_name);
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

  resolved_path file_system::deep_search_for_mount(const filepath& path) const {
    PROFILE_SECTION("file_system::deep_search_for_mount");
    std::lock_guard lock(fs_mutex);

    ref<directory> best = nullptr;
    size_t best_len = 0;
    for (const auto& [hash, mount] : mounts) {
      if (!mount->contains_path(path)) {
        continue;
      }

      const size_t len = normalize_lexical(mount->absolute_path()).size();
      if (best == nullptr || len > best_len) {
        best = mount;
        best_len = len;
      }
    }

    if (best == nullptr) {
      return {};
    }

    resolved_path rp;
    rp.mount_name = best->name();
    rp.scope = best->get_scope();
    rp.file_name = path.filename().stem().string();
    rp.extension = path.filename().extension().string();
    rp.relative_path_components = directory::split_path(std::filesystem::relative(path, best->absolute_path()).string());
    rp.relative_path_components.pop_back();
    return rp;
  }

  bool file_system::is_mounted(const std::string_view mount_name) const {
    std::lock_guard lock(fs_mutex);
    return mounts.find(FNV(mount_name)) != mounts.end();
  }

  void file_system::apply_watch_filter(const filepath& root_abs, const glob_set* set) {
    PROFILE_SECTION("file_system::apply_watch_filter");
    std::lock_guard lock(fs_mutex);
    const std::string target = normalize_lexical(std::filesystem::absolute(root_abs));

    /// deepest watched mount covering the domain root owns the filter
    ref<directory> best = nullptr;
    size_t best_len = 0;
    for (const auto& [hash, mount] : mounts) {
      if (!mount->watches_subtree()) {
        continue;
      }
      const std::string mount_root = normalize_lexical(mount->absolute_path());
      if (mount_root != target && !try_relative(target, mount_root).has_value()) {
        continue;
      }
      if (best == nullptr || mount_root.size() > best_len) {
        best = mount;
        best_len = mount_root.size();
      }
    }

    if (best == nullptr) {
      CORE_LOG_WARN("No watched mount covers '{}'; domain watch filter not applied", root_abs.string());
      return;
    }
    best->set_watch_filter(set);
  }

  ref<file_handle> file_system::get_file(const std::string_view engine_path) {
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

    ref<directory> target_dir = mount;
    if (!rp.relative_path_components.empty()) {
      if (ref<directory> walked = walk_path(mount, rp.relative_path_components); walked != nullptr) {
        target_dir = walked;
      }
    }

    if (ref<file_handle> file = target_dir->get_file(rp.file_name, rp.extension);
        file != nullptr) {
      return file;
    }

    filepath disk_path = mount->absolute_path();
    for (const auto& comp : rp.relative_path_components) {
      disk_path /= comp;
    }
    disk_path /= (rp.file_name + rp.extension);
    if (std::filesystem::exists(disk_path) && std::filesystem::is_regular_file(disk_path)) {
      PROFILE_SECTION("file_system::get_file--materialize_local");
      /// materialize the directory chain under the mount so later lookups cache-hit
      ref<directory> registry_dir = mount;
      if (!rp.relative_path_components.empty()) {
        registry_dir = walk_or_create_path(mount, rp.relative_path_components);
      }
      ref<local_file> local = create_local_file(disk_path);
      registry_dir->add_file(local);
      return local;
    }

    CORE_LOG_ERROR("Cannot get file '{}': file '{}' not found in mount '{}'", engine_path, rp.file_name, rp.mount_name);
    return nullptr;
  }

  ostd::vector<std::string> file_system::mounted_names() const {
    PROFILE_SECTION("file_system::mounted_names");
    std::lock_guard lock(fs_mutex);
    ostd::vector<std::string> names;
    names.reserve(mounts.size());
    for (const auto& [hash, dir] : mounts) {
      names.push_back(dir->name());
    }
    return names;
  }

  bool file_system::path_exists(const std::string_view engine_path) const {
    return file_exists(engine_path) || directory_exists(engine_path);
  }

  bool file_system::file_exists(const std::string_view engine_path) const {
    PROFILE_SECTION("file_system::file_exists");
    bool is_system_file = std::filesystem::exists(filepath(engine_path)) && std::filesystem::is_regular_file(filepath(engine_path));
    if (is_system_file) {
      return true;
    }

    resolved_path rp = resolve_path(engine_path);
    if (!rp.is_valid() || rp.file_name.empty()) {
      return false;
    }

    ref<directory> mount = get_mount(rp.mount_name);
    if (mount == nullptr) {
      return false;
    }

    /// tolerate an unindexed directory chain: a failed walk just skips the cache check
    ///  and falls through to the disk probe
    ref<directory> target_dir = mount;
    if (!rp.relative_path_components.empty()) {
      target_dir = walk_path(mount, rp.relative_path_components);
    }

    if (target_dir != nullptr) {
      for (const auto& [hash, file] : target_dir->get_files()) {
        if (file->name() == rp.file_name && (rp.extension.empty() || file->extension() == rp.extension)) {
          return true;
        }
      }
    }

    if (!mount->absolute_path().empty()) {
      PROFILE_SECTION("file_system::file_exists--disk_probe");
      filepath disk_path = mount->absolute_path();
      for (const auto& comp : rp.relative_path_components) {
        disk_path /= comp;
      }
      disk_path /= (rp.file_name + rp.extension);
      return std::filesystem::exists(disk_path) && std::filesystem::is_regular_file(disk_path);
    }

    return false;
  }

  bool file_system::directory_exists(const std::string_view engine_path) const {
    PROFILE_SECTION("file_system::directory_exists");
    for (const auto& [hash, mount] : mounts) {
      if (mount->directory_exists(engine_path)) {
        return true;
      }
    }
    return false;
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

    /// resolve_path already split the file name off; relative_path_components holds
    ///  only the directory chain (empty for mount://file.ext)
    std::span<const std::string> components = rp.relative_path_components;
    if (rp.file_name.empty()) {
      CORE_LOG_ERROR("Cannot open '{}': no file name in path", engine_path);
      return nullptr;
    }

    /// the directory chain may not be indexed yet (mounting does not scan the disk),
    ///  so a failed walk falls through to the disk probe below
    ref<directory> target_dir = mount;
    if (!components.empty()) {
      target_dir = walk_path(mount, components);
    }

    ref<file_handle> file = target_dir != nullptr ? target_dir->get_file(rp.file_name, rp.extension) : nullptr;
    if (file == nullptr) {
      PROFILE_SECTION("file_system::open--disk_fallback");
      /// try to open it as a local file if the mount has a disk path
      if (!mount->absolute_path().empty()) {
        filepath disk_path = mount->absolute_path();
        for (const auto& comp : components) {
          disk_path /= comp;
        }
        disk_path /= (rp.file_name + rp.extension);

        if (std::filesystem::exists(disk_path) && std::filesystem::is_regular_file(disk_path)) {
          auto local = make_ref<local_file>(*events, disk_path, engine_path);
          local->parent = (target_dir != nullptr ? target_dir : mount).raw_ptr();
          return local;
        }
      }

      CORE_LOG_ERROR("Cannot open '{}': file '{}{}' not found in mount '{}'", engine_path, rp.file_name, rp.extension, rp.mount_name);
      return nullptr;
    }

    return file;
  }

  ref<local_file> file_system::register_local_file(const filepath& path) {
    OTHER_ASSERT(events != nullptr, "File system events not initialized when registering local file for path: {}", path.string());
    PROFILE_SECTION("file_system::register_local_file");

    filepath abs_path = std::filesystem::absolute(path);
    OTHER_ASSERT(std::filesystem::exists(abs_path) && std::filesystem::is_regular_file(abs_path), "Cannot register local file: path '{}' does not exist or is not a regular file", abs_path.string());

    /// register under the deepest existing mount so virtual_path/stable_id match the resolver;
    //  minting a mount per parent dir made virtualize() output depend on load history and collide
    const resolved_path rp = deep_search_for_mount(abs_path);
    if (rp.is_valid()) {
      ref<directory> mount = get_mount(rp.mount_name);
      OTHER_ASSERT(mount != nullptr, "Resolved mount '{}' vanished while registering local file '{}'", rp.mount_name, path.string());

      ref<directory> target_dir = mount;
      if (!rp.relative_path_components.empty()) {
        target_dir = walk_or_create_path(mount, rp.relative_path_components);
      }
      OTHER_ASSERT(target_dir != nullptr, "Failed to walk directory chain for local file '{}'", path.string());
      CORE_LOG_DEBUG("Registering local file '{}' under mount '{}'", path.string(), rp.mount_name);

      ref<local_file> local = create_local_file(abs_path);
      OTHER_ASSERT(local != nullptr, "Failed to create local file for path: {}", path.string());
      target_dir->add_file(local);
      return local;
    }

    /// outside every mount: keep it reachable as a toplevel file, no new mounts
    ref<local_file> local = create_local_file(abs_path);
    OTHER_ASSERT(local != nullptr, "Failed to create local file for path: {}", path.string());
    add_toplevel_file(local);
    return local;
  }

  ref<virtual_file> file_system::create_asset_virtual_file(const std::string_view virtual_path) {
    OTHER_ASSERT(events != nullptr, "File system events not initialized when creating asset virtual file for path: {}", virtual_path);
    return make_ref<virtual_file>(*events, virtual_path);
  }

  ref<virtual_file> file_system::create_virtual_file(const std::string_view mount_name, const std::string_view relative_path, ostd::vector<uint8_t>&& initial_data) {
    PROFILE_SECTION("file_system::create_virtual_file");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot create virtual file: mount '{}' not found", mount_name);
      return nullptr;
    }

    ostd::vector<std::string> components = directory::split_path(relative_path);
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

    auto vfile = make_ref<virtual_file>(*events, name, ext, std::move(initial_data));
    vfile->parent = target_dir.raw_ptr();

    /// build the virtual path
    std::string vpath = std::string(mount_name) + std::string(kPathSeparator) + std::string(relative_path);

    target_dir->add_file(vfile);

    CORE_LOG_DEBUG("Created virtual file '{}' in mount '{}'", vpath, mount_name);
    return vfile;
  }

  ref<virtual_file> file_system::create_virtual_file(const std::string_view mount_name, const std::string_view file_name, const std::string_view ext, ostd::vector<uint8_t>&& initial_data) {
    PROFILE_SECTION("file_system::create_virtual_file");

    ref<directory> mount = get_mount(mount_name);
    if (mount == nullptr) {
      CORE_LOG_ERROR("Cannot create virtual file: mount '{}' not found", mount_name);
      return nullptr;
    }

    std::string full_name = std::string(file_name) + std::string(ext);
    auto vfile = make_ref<virtual_file>(*events, full_name, ext, std::move(initial_data));
    vfile->parent = mount.raw_ptr();

    mount->add_file(vfile);

    CORE_LOG_DEBUG("Created virtual file '{}{}' in mount root '{}'", file_name, ext, mount_name);
    return vfile;
  }

  ref<remote_file> file_system::register_remote_file(
    const std::string_view mount_name,
    const std::string_view relative_path,
    const std::string_view url) {
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

    auto rfile = make_ref<remote_file>(*events, name, ext, url);
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

  ref<local_file> file_system::create_local_file(const filepath& path) {
    OTHER_ASSERT(events != nullptr, "File system events not initialized when creating local file for path: {}", path.string());
    return make_ref<local_file>(*events, path, path.string());
  }

  ref<directory> file_system::walk_or_create_path(ref<directory> root, const std::span<const std::string> components) {
    PROFILE_SECTION("file_system::walk_or_create_path");
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

  ref<directory> file_system::walk_path(ref<directory> root, const std::span<const std::string> components) const {
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
    PROFILE_SECTION("file_system::scan_directory_impl");
    if (!std::filesystem::exists(disk_path) || !std::filesystem::is_directory(disk_path)) {
      return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(disk_path)) {
      if (entry.is_regular_file()) {
        auto local = make_ref<local_file>(*events, entry.path(), filepath{ dir->absolute_path() / entry.path().filename() }.string());
        dir->add_file(local);
      } else if (entry.is_directory() && recursive) {
        std::string child_name = entry.path().filename().string();
        auto child_dir = dir->add_child_directory(child_name, entry.path());
        scan_directory_impl(child_dir, entry.path(), recursive);
      }
    }
  }

}  // namespace other