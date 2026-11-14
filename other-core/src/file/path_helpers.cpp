/**
 * \file file/path_helpers.cpp
 **/
#include "file/path_helpers.hpp"

#include "core/defines.hpp"
#include "file/filesystem.hpp"

namespace other {
  namespace {

    static filepath environment_install_prefix() {
      return filepath{ get_tag_replacement("${other-directory}") };  /// "C:/OtherEnvironment"
    }

    static filepath environment_build_root() {
      /// dev builds run under <repo>/build/...; walk up from the exe looking for 'build'.
      ///  installed builds have no such ancestor (empty -> install prefix classifies instead)
      filepath dir = get_current_exe_directory();
      while (!dir.empty() && dir != dir.parent_path()) {
        if (dir.filename() == "build") {
          return dir;
        }
        dir = dir.parent_path();
      }
      return {};
    }

    static opt<filepath> resolve_mount_qualified(const std::string_view expanded) {
      const size_t sep = expanded.find(file_system::kPathSeparator);
      if (sep == std::string_view::npos) {
        return std::nullopt;
      }

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system unavailable");

      ref<directory> mount = fs->get_mount(expanded.substr(0, sep));
      if (mount == nullptr) {
        return std::nullopt;
      }

      const std::string_view rel = expanded.substr(sep + file_system::kPathSeparator.size());
      return (mount->absolute_path() / filepath{ std::string{ rel } }).lexically_normal();
    }

    static ostd::vector<filepath> search_roots(const filepath& referencing_file) {
      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system unavailable");

      ostd::vector<filepath> roots = {};
      if (!referencing_file.empty()) {
        roots.push_back(dir_of(std::filesystem::absolute(referencing_file)));
      }

      /// \todo needs a small accessor on file_system: roots of every mount_scope::PROJECT mount
      for (const filepath& root : fs->project_mount_roots()) {
        roots.push_back(root);
      }
      roots.push_back(file_system::get_cwd());

      if (ref<directory> engine = fs->get_mount(file_system::kEngineMount); engine != nullptr) {
        roots.push_back(engine->absolute_path());
      }
      return roots;
    }

  }  // namespace

  std::string to_mount_qualified(const filepath& abs) {
    /// virtualize yields "mount/rel/path.ext"; swap the first '/' for "://"
    std::string vp = virtualize(abs);
    const size_t slash = vp.find('/');
    OTHER_ASSERT(slash != std::string::npos, "virtualize produced a path with no mount segment: '{}'", vp);
    vp.replace(slash, 1, file_system::kPathSeparator);
    return vp;
  }

  asset_path resolve_asset_path(const std::string_view raw, const filepath& referencing_file) {
    PROFILE_SECTION("resolve_asset_path");
    asset_path result = { .raw = std::string{ raw } };
    if (raw.empty()) {
      return result;
    }

    std::string expanded = perform_tag_replacement(raw);
    std::ranges::replace(expanded, '\\', '/');

    if (opt<filepath> mounted = resolve_mount_qualified(expanded); mounted.has_value()) {
      if (std::filesystem::exists(*mounted)) {
        result.resolved = *mounted;
      } else {
        CORE_LOG_ERROR("Asset path '{}' does not exist under its mount ({})", raw, mounted->string());
      }
      return result;
    }

    const filepath candidate{ expanded };
    if (candidate.is_absolute()) {
      if (std::filesystem::exists(candidate)) {
        result.resolved = candidate.lexically_normal();
      } else {
        CORE_LOG_ERROR("Asset path '{}' does not exist ({})", raw, candidate.string());
      }
      return result;
    }

    const ostd::vector<filepath> roots = search_roots(referencing_file);
    for (const filepath& root : roots) {
      const filepath attempt = (root / candidate).lexically_normal();
      if (std::filesystem::exists(attempt)) {
        result.resolved = attempt;
        return result;
      }
    }

    CORE_LOG_ERROR("Asset path '{}' not found; searched {} roots (first: {})", raw, roots.size(), roots.empty() ? "<none>" : roots.front().string());
    return result;
  }

  std::string normalize_lexical(const filepath& p) {
    std::string s = p.lexically_normal().generic_string();
    if (s.size() > 1 && s.back() == '/') s.pop_back();
    return s;
  }

  opt<std::string> try_relative(const filepath& abs, const filepath& root_abs) {
    const std::string a = normalize_lexical(abs);
    const std::string r = normalize_lexical(root_abs);
    if (a.size() <= r.size() + 1) {
      return std::nullopt;
    }

    const std::string_view prefix{ a.data(), r.size() };
#if OTHER_ENVIRONMENT_WINDOWS
    if (!detail::equals_case_insensitive(prefix, r)) {
      return std::nullopt;
    }
#else
    if (prefix != r) {
      return std::nullopt;
    }
#endif

    if (a[r.size()] != '/') {
      return std::nullopt;
    }
    return a.substr(r.size() + 1);
  }

  filepath dir_of(const filepath& file) {
    const filepath parent = file.parent_path();
    OTHER_ASSERT(!parent.empty(), "'{}' has no parent directory", file.string());
    return parent;
  }

  filepath resolve_relative(const filepath& base_file, std::string_view rel) {
    std::string fixed{ rel };
    std::ranges::replace(fixed, '\\', '/');
    const filepath p{ fixed };
    return (p.is_absolute() ? p : dir_of(base_file) / p).lexically_normal();
  }

  std::string virtualize(const filepath& abs) {
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "file_system subsystem unavailable in virtualize");

    const resolved_path rp = fs->deep_search_for_mount(abs);
    OTHER_ASSERT(rp.is_valid(), "'{}' is not under any mount - cannot be a dependency", abs.string());

    std::string vp{ rp.mount_name };
    for (const std::string& piece : rp.relative_path_components) {
      vp += '/';
      vp += piece;
    }
    vp += '/';
    vp += rp.file_name;
    vp += rp.extension;

    OTHER_ASSERT(!vp.empty() && vp.front() != '/' && vp.find('\\') == std::string::npos &&
                   vp.find("..") == std::string::npos,
                 "virtualize produced non-canonical '{}'", vp);
    return vp;
  }

  filepath absolute_of(std::string_view virtual_path) {
    const size_t slash = virtual_path.find('/');
    OTHER_ASSERT(slash != std::string_view::npos,
                 "canonical virtual paths start with a mount segment: '{}'", virtual_path);

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "file_system subsystem unavailable in absolute_of");

    ref<directory> mount = fs->get_mount(virtual_path.substr(0, slash));
    OTHER_ASSERT(mount != nullptr, "unknown mount in virtual path '{}'", virtual_path);
    return mount->absolute_path() / filepath{ std::string{ virtual_path.substr(slash + 1) } };
  }

  bool is_under_any_project_mount(const filepath& abs) {
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "file_system subsystem unavailable in is_under_any_project_mount");
    const resolved_path rp = fs->deep_search_for_mount(abs);
    return rp.is_valid() && rp.scope == mount_scope::PROJECT;
  }

  bool is_environment_owned(const filepath& abs) {
    static const std::array<filepath, 2> roots = {
      environment_build_root(),
      environment_install_prefix(),
    };
    return std::ranges::any_of(roots, [&](const filepath& r) { return !r.empty() && try_relative(abs, r).has_value(); });
  }

  filepath environment_root() {
    const filepath r = environment_build_root();
    if (!r.empty()) {
      return r.parent_path();
    }
    return get_other_environment_install_folder();
  }

  bool is_installed_build() {
    return environment_build_root().empty();
  }

}  // namespace other