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

    filepath folder_path = "";
#if 0
    /// get OtherEnvironment install folder
    ///   windows: PROGRAMFILES/OtherEnvironment
    ///   linux: /usr/local/OtherEnvironment
    /// \todo mac
  #ifdef OTHER_ENVIRONMENT_WINDOWS
    char* program_files = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&program_files, &len, "PROGRAMFILES");
    OTHER_ASSERT(err == 0 && program_files != nullptr, "Failed to get PROGRAMFILES environment variable.");
    folder_path = filepath(program_files) / filepath("OtherEnvironment");
    free(program_files);
  #elif defined(OTHER_ENVIRONMENT_UNIX)
    #error "Unimplemented"
  #else
    #error "Unsupported platform"
  #endif
#else
    /// this only works in the dev environment on my machine right now, need to fix later
    folder_path = std::filesystem::current_path();
#endif
    return folder_path;
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

}  // namespace other