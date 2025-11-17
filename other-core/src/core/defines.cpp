/**
 * \file core/defines.cpp
 **/
#include "core/defines.hpp"

#include "core/logger.hpp"

namespace other {

  filepath get_program_files_folder(const std::string_view app_name) {
    /// get program files folder
    ///   windows: PROGRAMFILES/OtherEngine
    ///   linux: /usr/local/OtherEngine
    /// \todo mac

    filepath folder_path = "";
#ifdef OTHER_ENVIRONMENT_WINDOWS
    char* program_files = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&program_files, &len, "PROGRAMFILES");
    OTHER_ASSERT(err == 0 && program_files != nullptr, "Failed to get PROGRAMFILES environment variable.");
    folder_path = filepath(program_files) / filepath(app_name);
    free(program_files);
#elif defined(OTHER_ENVIRONMENT_UNIX)
  #error "Unimplemented"
#else
  #error "Unsupported platform"
#endif

    return folder_path;
  }

  filepath get_app_data_folder(const std::string_view app_name, bool create) {
    /// get app folder
    ///   windows: APPDATA/OtherServer
    ///   linux: ~/.otherserver
    /// \todo mac

    filepath folder_path = "";
#ifdef OTHER_ENVIRONMENT_WINDOWS
    char* appdata = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&appdata, &len, "APPDATA");
    OTHER_ASSERT(err == 0 && appdata != nullptr, "Failed to get APPDATA environment variable.");
    folder_path = filepath(appdata) / filepath(app_name);
    free(appdata);
#elif defined(OTHER_ENVIRONMENT_UNIX)
    const char* home = std::getenv("HOME");
    OTHER_ASSERT(home != nullptr, "Failed to get HOME environment variable.");
    folder_path = filepath(home);
    folder_path /= filepath("." + std::string{ app_name } | std::views::transform([](char c) { return std::tolower(c); }) | std::ranges::to<std::string>());
#else
  #error "Unsupported platform"
#endif

    if (create && !std::filesystem::exists(folder_path)) {
      std::filesystem::create_directories(folder_path);
    }

    return folder_path;
  }

  filepath get_other_environment_install_folder() {
    /// \todo: implement this and add a development override to return local dev dir
    ///         instead of prod installation path, currently returning hard coded local dev folder

#if 1
  /// get OtherEnvironment install folder
  ///   windows: PROGRAMFILES/OtherEnvironment
  ///   linux: /usr/local/OtherEnvironment
  /// \todo mac
  #ifdef OTHER_ENVIRONMENT_WINDOWS
    return filepath("C:/OtherEnvironment");
  #elif defined(OTHER_ENVIRONMENT_UNIX)
    return filepath("/usr/local/OtherEnvironment");
  #else
    #error "Unsupported platform"
  #endif
#else
    filepath folder_path = "";
    /// this only works in the dev environment on my machine right now, need to fix later
    folder_path = std::filesystem::current_path();
    return folder_path;
#endif
  }

  filepath get_system_default_working_directory() {
    /// get system default working directory
    ///   windows: C:/Users/<username>/Documents/OtherEngine
    ///   linux: /home/<username>/OtherEngine
    /// \todo mac

    filepath folder_path = "";
#ifdef OTHER_ENVIRONMENT_WINDOWS
    char* documents = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&documents, &len, "USERPROFILE");
    OTHER_ASSERT(err == 0 && documents != nullptr, "Failed to get USERPROFILE environment variable.");
    folder_path = filepath(documents) / "Documents" / "OtherEngine";
    free(documents);
#elif defined(OTHER_ENVIRONMENT_UNIX)
    const char* home = std::getenv("HOME");
    OTHER_ASSERT(home != nullptr, "Failed to get HOME environment variable.");
    folder_path = filepath(home) / "OtherEngine";
#else
  #error "Unsupported platform"
#endif

    if (!std::filesystem::exists(folder_path)) {
      std::filesystem::create_directories(folder_path);
    }

    return folder_path;
  }

  std::string get_tag_replacement(const std::string_view tag) {
    if (!tag.starts_with("${") || !tag.ends_with("}")) {
      return std::string{ tag };
    }

    std::string inner_tag = std::string{ tag.substr(2, tag.size() - 3) };

    /// folders and stuff, most users won't care for these
    if (inner_tag == "other-directory") {
      return "C:/OtherEnvironment";
    }
    if (inner_tag == "program-files") {
      return get_program_files_folder("OtherEnvironment").string();
    }
    if (inner_tag == "app-data") {
      return get_app_data_folder("OtherEnvironment", true).string();
    }
    if (inner_tag == "default-working-directory") {
      return get_system_default_working_directory().string();
    }

    /// build config values
    if (inner_tag == "build-config") {
#if defined(OTHER_DEBUG_BUILD)
      return "Debug";
#elif defined(OTHER_RELEASE_BUILD)
      return "Release";
#elif defined(OTHER_PROFILE_BUILD)
      return "Profile";
#elif defined(OTHER_PROFILED_BUILD)
      return "ProfileD";
#endif
    }

    /// return the whole tag with no changes if we have no replacement
    return std::string{ tag };
  }

  namespace {

    static bool has_tags(const std::string_view str) {
      return str.find("${") != std::string_view::npos;
    }

    static bool tags_are_valid(const std::string_view str) {
      size_t pos = 0;
      while (true) {
        size_t start_pos = str.find("${", pos);
        if (start_pos == std::string_view::npos) {
          break;
        }
        size_t end_pos = str.find("}", start_pos);
        if (end_pos == std::string_view::npos) {
          return false;
        }
        pos = end_pos + 1;
      }
      return true;
    }

  }  // namespace

  std::string perform_tag_replacement(const std::string_view tag) {
    if (!has_tags(tag)) {
      return std::string{ tag };
    }

    if (!tags_are_valid(tag)) {
      CORE_LOG_ERROR("Tag in string '{}' is invalid, missing closing '}}'", tag);
      return std::string{ tag };
    }

    std::string result;
    size_t pos = 0;

    do {
      size_t start_pos = tag.find("${", pos);
      if (start_pos == std::string_view::npos) {
        result += tag.substr(pos);
        break;
      }

      result += tag.substr(pos, start_pos - pos);
      size_t end_pos = tag.find("}", start_pos);
      if (end_pos == std::string_view::npos) {
        result += tag.substr(start_pos);
        break;
      }

      /// get_tag_replacement takes whole tag including ${ and }
      std::string_view inner_tag = tag.substr(start_pos, end_pos - start_pos + 1);
      result += get_tag_replacement(inner_tag);

      pos = end_pos + 1;
    } while (pos < tag.size());
    return result;
  }

}  // namespace other