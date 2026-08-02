/**
 * \file file/path_helpers.hpp
 **/
#ifndef OTHER_CORE_FILE_PATH_HELPERS_HPP
#define OTHER_CORE_FILE_PATH_HELPERS_HPP

#include <string>

namespace other {
  namespace detail {

    inline std::string_view trim(std::string_view s) {
      while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
      while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
      return s;
    }

    inline ostd::vector<std::string_view> split(std::string_view s, char sep) {
      ostd::vector<std::string_view> out;
      size_t start = 0;
      while (start <= s.size()) {
        const size_t pos = s.find(sep, start);
        out.push_back(s.substr(start, pos == std::string_view::npos ? std::string_view::npos : pos - start));
        if (pos == std::string_view::npos) break;
        start = pos + 1;
      }
      return out;
    }

    constexpr char fold_case(char c) {
      return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    constexpr bool chars_equal(char a, char b) {
#if OTHER_ENVIRONMENT_WINDOWS
      return fold_case(a) == fold_case(b);
#else
      return a == b;
#endif
    }

    inline bool equals_case_insensitive(std::string_view a, std::string_view b) {
      return a.size() == b.size() &&
        std::ranges::equal(a, b, [](char x, char y) { return fold_case(x) == fold_case(y); });
    }

  }  // namespace detail

  std::string normalize_lexical(const filepath& p);

  opt<std::string> try_relative(const filepath& path, const filepath& base);

  filepath dir_of(const filepath& path);
  filepath resolve_relative(const filepath& base, std::string_view relative_path);

  std::string virtualize(const filepath& path);
  filepath absolute_of(std::string_view virtual_path);

  bool is_under_any_project_mount(const filepath& path);
  bool is_environment_owned(const filepath& path);

}  // namespace other

#endif  // OTHER_CORE_FILE_PATH_HELPERS_HPP