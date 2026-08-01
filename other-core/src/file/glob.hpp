/**
 * \file file/glob.hpp
 **/
#ifndef OTHER_CORE_FILE_GLOB_HPP
#define OTHER_CORE_FILE_GLOB_HPP

namespace other {

  class glob_set {
   public:
    glob_set() = default;
    ~glob_set() = default;

    glob_set& include(std::string_view pattern);
    glob_set& exclude(std::string_view pattern);
    glob_set& exclude(std::initializer_list<std::string_view> patterns);

    bool matches(std::string_view relative_path) const;
    bool excluded(std::string_view relative_path) const;
    bool may_contain(std::string_view relative_dir) const;
    natural_t content_hash() const;

   private:
    struct glob_pattern {
      std::string source;
      ostd::vector<std::string> segments;
    };

    ostd::vector<glob_pattern> includes;
    ostd::vector<glob_pattern> excludes;

    static glob_pattern compile_pattern(std::string_view pattern);
    static bool match_segment(std::string_view pattern, std::string_view text);
    static bool match_segments(std::span<const std::string> pat, std::span<const std::string_view> path);
    static bool prefix_viable(std::span<const std::string> pat, std::span<const std::string_view> dir);
    static bool covers_subtree(std::span<const std::string> pat, std::span<const std::string_view> dir);
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_GLOB_HPP