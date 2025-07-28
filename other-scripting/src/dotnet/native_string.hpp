/**
 * \file dotnet/native_string.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_NATIVE_STRING_HPP
#define OTHER_SCRIPTING_DOTNET_NATIVE_STRING_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include <dotnet/nethost.h>

namespace other {

  namespace detail {

    std::string convert_string(const std::basic_string_view<char_t>& str);

  }  // namespace detail

  class native_string {
   public:
    native_string() = default;
    ~native_string() = default;

    static native_string new_str(const char* str);
    static native_string new_str(std::string_view str);

    static void free_str(native_string& str);

    void assign(const std::string_view str);
    native_string(const std::string_view str) {
      assign(str);
    }
    native_string& operator=(const std::string_view str) {
      assign(str);
      return *this;
    }

    operator std::string() const;
    bool operator==(const native_string& other) const;
    bool operator==(std::string_view other) const;

    char_t* data() { return raw_string; }
    const char_t* data() const { return raw_string; }

   private:
    char_t* raw_string = nullptr;
    uint32_t disposed = false;
  };

  class native_scoped_string {
   public:
    native_scoped_string(native_string string)
        : string(std::move(string)) {}
    ~native_scoped_string() {
      native_string::free_str(string);
    }

    native_scoped_string& operator=(native_string str) {
      native_string::free_str(string);
      string = std::move(str);
      return *this;
    }

    native_scoped_string& operator=(const native_string& str) {
      native_string::free_str(string);
      string = str;
      return *this;
    }

    native_scoped_string& operator=(const native_scoped_string& str) {
      native_string::free_str(string);
      string = str.string;
      return *this;
    }

    operator std::string() const { return string; }
    operator native_string() const { return string; }

    bool operator==(const native_scoped_string& other) const { return string == other.string; }
    bool operator==(std::string_view other) const { return string == other; }

   private:
    native_string string;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_NATIVE_STRING_HPP