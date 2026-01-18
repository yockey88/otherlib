/**
 * \file dotnet/native_string.cpp
 **/
#include "dotnet/native_string.hpp"

#include "dotnet/dotnet_memory.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <Shlwapi.h>
  #include <Windows.h>
#else
#endif  // OTHER_ENVIRONMENT_WINDOWS

namespace other {
  namespace detail {

#ifdef OTHER_ENVIRONMENT_WINDOWS
    std::wstring convert_char_to_wide(const std::string_view str) {
      int32_t length = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.length()), nullptr, 0);
      std::wstring res(length, wchar_t{ 0 });
      MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.length()), res.data(), length);
      return res;
    }

    std::string convert_wide_to_char(const std::wstring_view str) {
      int32_t length = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.length()), nullptr, 0, nullptr, nullptr);
      std::string res(length, char{ 0 });
      WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.length()), res.data(), length, nullptr, nullptr);
      return res;
    }
#else
    std::string convert_char_to_wide(const std::string_view str) {
      return std::string(str);
    }

    std::string convert_wide_to_char(const std::wstring_view str) {
      return std::string(str.begin(), str.end());
    }
#endif  // OTHER_ENVIRONMENT_WINDOWS

    std::string convert_string(const std::basic_string_view<char_t>& str) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      return convert_wide_to_char(std::wstring_view(str));
#else
      return std::string(str);
#endif
    }

    std::basic_string<char_t> convert_string(const std::string& str) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      std::wstring wide_str = convert_char_to_wide(str);
      return std::basic_string<char_t>(wide_str);
#else
      return std::basic_string<char_t>(str);
#endif
    }

  }  // namespace detail

  native_string native_string::new_str(const char* str) {
    native_string result;
    result.assign(str);
    return result;
  }

  native_string native_string::new_str(std::string_view str) {
    native_string result;
    result.assign(str);
    return result;
  }

  void native_string::free_str(native_string& str) {
    if (str.raw_string == nullptr) {
      return;
    }

    dotnet_memory::free_co_task_mem(str.raw_string);
    str.raw_string = nullptr;
  }

  void native_string::assign(const std::string_view str) {
    if (raw_string != nullptr) {
      dotnet_memory::free_co_task_mem(raw_string);
    }

    raw_string = dotnet_memory::native_string_to_co_task_mem(detail::convert_char_to_wide(str));
  }

  native_string::operator std::string() const {
    if (raw_string == nullptr) {
      return "";
    }

    std::basic_string<char_t> str(raw_string);
    return detail::convert_wide_to_char(str);
  }

  bool native_string::operator==(const native_string& other) const {
    if (raw_string == other.raw_string) {
      return true;
    }

    if (raw_string == nullptr || other.raw_string == nullptr) {
      return false;
    }

    return wcscmp(raw_string, other.raw_string) == 0;
  }

}  // namespace other