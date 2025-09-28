/**
 * \file core/defines.cpp
 **/
#include "core/defines.hpp"

#include "core/logger.hpp"

namespace other {

  filepath get_app_data_folder(const std::string_view app_name) {
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

    if (!std::filesystem::exists(folder_path)) {
      std::filesystem::create_directories(folder_path);
    }

    return folder_path;
  }

  filepath get_project_cache(const std::string_view other_folder) {
    filepath cache_path = get_app_data_folder(other_folder) / "project_cache.json";
    if (!std::filesystem::exists(cache_path)) {
      std::ofstream file(cache_path);
      OTHER_ASSERT(file.is_open(), "Failed to create project cache file at {}", cache_path.string());
      file << "{}";
      file.close();
    }
    return cache_path;
  }

}  // namespace other