/**
 * \file platform/windows/windows_library_handle.cpp
 **/
#include "windows_library_handle.hpp"

#include <codecvt>
#include <locale>
#include <string>

#include "core/logger.hpp"

namespace other {

  void windows_library_handle::load() {
    std::wstring wpath = std::wstring(std::begin(filepath), std::end(filepath));

    CORE_LOG_DEBUG("Loading library '{}' [cwd : {}]", filepath, std::filesystem::current_path().string());
    handle = LoadLibraryW(wpath.c_str());
    if (handle == nullptr) {
      CORE_LOG_ERROR("Failed to load library '{}'", filepath);
      print_error();
    } else {
      CORE_LOG_DEBUG("Successfully loaded library '{}'", filepath);
    }
  }

  bool windows_library_handle::is_loaded() const {
    return handle != nullptr;
  }

  void windows_library_handle::unload() {
    if (handle != nullptr) {
      if (!FreeLibrary(handle)) {
        CORE_LOG_ERROR("Failed to unload library '{}'", filepath);
        print_error();
      } else {
        CORE_LOG_DEBUG("Successfully unloaded library '{}'", filepath);
      }
      handle = nullptr;
    }
  }

  symbol windows_library_handle::load_symbol(const std::string_view sym_name) {
    if (handle == nullptr) {
      CORE_LOG_ERROR("Library handle is null");
      return {
        .address = nullptr,
      };
    }

    symbol sym = {
      .address = (void*)GetProcAddress(handle, sym_name.data()),
    };
    if (sym.address == nullptr) {
      CORE_LOG_ERROR("Failed to load symbol '{}' from library '{}'", sym_name, filepath);
      print_error();
    }
    return sym;
  }

  void windows_library_handle::print_error() const {
    DWORD error_code = GetLastError();
    LPVOID error_message;
    FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&error_message), 0, nullptr
    );

    std::wstring error_wstr(static_cast<wchar_t*>(error_message));
    LocalFree(error_message);

    // setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    // use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    std::string error_str = converter.to_bytes(error_wstr);

    CORE_LOG_ERROR("Windows Error [{}] :\n\t!> {}", error_code, error_str);
  }

}  // namespace other