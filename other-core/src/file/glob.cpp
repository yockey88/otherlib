/**
 * \file file/glob.cpp
 **/
#include "file/glob.hpp"

#include <xxHash/xxh3.h>

#include "file/path_helpers.hpp"

namespace other {
  namespace {

    ostd::vector<std::string_view> split_path(std::string_view relative_path) {
      OTHER_ASSERT(!relative_path.empty() && relative_path.front() != '/',
                   "expected root-relative path, got '{}'", relative_path);
      OTHER_ASSERT(relative_path.find('\\') == std::string_view::npos &&
                     relative_path.find("..") == std::string_view::npos,
                   "expected canonical virtual-path form, got '{}'", relative_path);
      return detail::split(relative_path, '/');
    }

  }  // namespace

  glob_set& glob_set::include(std::string_view pattern) {
    includes.push_back(compile_pattern(pattern));
    return *this;
  }

  glob_set& glob_set::exclude(std::string_view pattern) {
    excludes.push_back(compile_pattern(pattern));
    return *this;
  }

  glob_set& glob_set::exclude(std::initializer_list<std::string_view> patterns) {
    for (const std::string_view p : patterns) {
      exclude(p);
    }
    return *this;
  }

  bool glob_set::matches(std::string_view relative_path) const {
    const auto segments = split_path(relative_path);
    const bool included = std::ranges::any_of(includes, [&](const glob_pattern& p) {
      return match_segments(p.segments, segments);
    });
    return included && !excluded(relative_path);
  }

  bool glob_set::excluded(std::string_view relative_path) const {
    const auto segments = split_path(relative_path);
    return std::ranges::any_of(excludes, [&](const glob_pattern& p) {
      return match_segments(p.segments, segments);
    });
  }

  bool glob_set::may_contain(std::string_view relative_dir) const {
    const auto segments = split_path(relative_dir);
    const bool viable = std::ranges::any_of(includes, [&](const glob_pattern& p) {
      return prefix_viable(p.segments, segments);
    });
    if (!viable) return false;
    const bool fully_excluded = std::ranges::any_of(excludes, [&](const glob_pattern& p) {
      return covers_subtree(p.segments, segments);
    });
    return !fully_excluded;
  }

  natural_t glob_set::content_hash() const {
    auto sorted_sources = [](const ostd::vector<glob_pattern>& v) {
      ostd::vector<std::string_view> out;
      for (const glob_pattern& p : v) out.push_back(p.source);
      std::ranges::sort(out);
      return out;
    };

    std::string canon;
    for (const std::string_view s : sorted_sources(includes)) {
      canon += "i:";
      canon += s;
      canon += '\n';
    }
    for (const std::string_view s : sorted_sources(excludes)) {
      canon += "e:";
      canon += s;
      canon += '\n';
    }

    return XXH3_64bits(canon.data(), canon.size());
  }

  glob_set::glob_pattern glob_set::compile_pattern(std::string_view pattern) {
    OTHER_ASSERT(!pattern.empty(), "empty glob pattern");
    OTHER_ASSERT(pattern.find('\\') == std::string_view::npos, "glob patterns use forward slashes: '{}'", pattern);
    OTHER_ASSERT(pattern.front() != '/' && pattern.back() != '/', "glob patterns are root-relative with no trailing slash: '{}'", pattern);
    OTHER_ASSERT(pattern.find_first_of("{}[]") == std::string_view::npos, "unsupported glob syntax (braces/classes) in '{}'", pattern);

    glob_pattern out{ .source = std::string(pattern) };
    for (const std::string_view segment : detail::split(pattern, '/')) {
      OTHER_ASSERT(!segment.empty(), "empty segment in glob '{}'", pattern);
      if (segment.find("**") != std::string_view::npos) {
        OTHER_ASSERT(segment == "**", "'**' must be a whole segment: '{}'", pattern);
      }
      out.segments.emplace_back(segment);
    }
    return out;
  }

  bool glob_set::match_segment(std::string_view pattern, std::string_view text) {
    size_t p = 0, t = 0;
    size_t star = std::string_view::npos, star_t = 0;
    while (t < text.size()) {
      if (p < pattern.size() && (pattern[p] == '?' || detail::chars_equal(pattern[p], text[t]))) {
        ++p;
        ++t;
      } else if (p < pattern.size() && pattern[p] == '*') {
        star = p++;
        star_t = t;
      } else if (star != std::string_view::npos) {
        p = star + 1;
        t = ++star_t;
      } else {
        return false;
      }
    }
    while (p < pattern.size() && pattern[p] == '*') {
      ++p;
    }
    return p == pattern.size();
  }

  bool glob_set::match_segments(std::span<const std::string> pat, std::span<const std::string_view> path) {
    while (true) {
      if (pat.empty()) {
        return path.empty();
      }

      if (pat.front() == "**") {
        if (pat.size() == 1) {
          return true;
        }

        for (size_t skip = 0; skip <= path.size(); ++skip) {
          if (match_segments(pat.subspan(1), path.subspan(skip))) {
            return true;
          }
        }

        return false;
      }

      if (path.empty() || !match_segment(pat.front(), path.front())) {
        return false;
      }

      pat = pat.subspan(1);
      path = path.subspan(1);
    }
  }

  bool glob_set::prefix_viable(std::span<const std::string> pat, std::span<const std::string_view> dir) {
    while (!dir.empty()) {
      if (pat.empty()) {
        return false;
      }

      if (pat.front() == "**") {
        return true;
      }

      if (!match_segment(pat.front(), dir.front())) {
        return false;
      }

      pat = pat.subspan(1);
      dir = dir.subspan(1);
    }
    return !pat.empty();
  }

  bool glob_set::covers_subtree(std::span<const std::string> pat, std::span<const std::string_view> dir) {
    while (!dir.empty()) {
      if (pat.empty()) {
        return false;
      }

      if (pat.front() == "**") {
        return pat.size() == 1;
      }

      if (!match_segment(pat.front(), dir.front())) {
        return false;
      }

      pat = pat.subspan(1);
      dir = dir.subspan(1);
    }
    return pat.size() == 1 && pat.front() == "**";
  }

}  // namespace other