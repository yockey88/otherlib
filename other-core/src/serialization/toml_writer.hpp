/**
 * \file serialization/toml_writer.hpp
 *
 * write-side sibling of config_table (the toml++ read side). the writer owns document
 * layout — emission order, comments, blank lines — because toml::table is an alphabetical
 * std::map and cannot represent either. a parallel toml++ tree is built purely for debug
 * validation: finalize() re-parses the emitted text and asserts equivalence with it.
 *
 * misuse (duplicate keys, duplicate table headers, malformed values) is fatal via
 * OTHER_ASSERT, matching house contracts; environmental failures (save()) return bool.
 */
#ifndef OTHER_CORE_SERIALIZATION_TOML_WRITER_HPP
#define OTHER_CORE_SERIALIZATION_TOML_WRITER_HPP

#include <set>
#include "data-structures/std_container.hpp"
#include <sstream>
#include <string>

#include <toml++/toml.h>

#include "core/defines.hpp"

namespace other {

  struct toml_format_options {
    size_t indent_width = 0;  /// scene documents emit flush-left like project files
    bool space_inside_inline_braces = true;
  };

  class toml_writer {
   public:
    explicit toml_writer(toml_format_options options = {});

    /// -- document structure, in call order ---------------------------------
    toml_writer& comment(std::string_view text);
    toml_writer& blank();
    /// "[a.b]" — duplicate header for the same dotted path is fatal
    toml_writer& table(std::string_view dotted_path);
    /// "[[a.b]]" — repeatable
    toml_writer& table_array(std::string_view dotted_path);

    /// -- keys --------------------------------------------------------------
    toml_writer& key(std::string_view key_name, bool v);
    toml_writer& key(std::string_view key_name, int64_t v);
    toml_writer& key(std::string_view key_name, uint64_t v);
    toml_writer& key(std::string_view key_name, double v);
    /// floats emit their shortest round-trip representation ("0.2", not the exact
    ///  double expansion "0.20000000298...")
    toml_writer& key(std::string_view key_name, float v);
    toml_writer& key(std::string_view key_name, std::string_view v);
    toml_writer& key(std::string_view key_name, const char* v) { return key(key_name, std::string_view{ v }); }
    toml_writer& key(std::string_view key_name, const std::string& v) { return key(key_name, std::string_view{ v }); }
    toml_writer& key(std::string_view key_name, const filepath& v) { return key(key_name, std::string_view{ v.generic_string() }); }

    template <std::integral T>
      requires(!std::same_as<T, bool> && !std::same_as<T, int64_t> && !std::same_as<T, uint64_t>)
    toml_writer& key(std::string_view key_name, T v) {
      if constexpr (std::is_signed_v<T>) {
        return key(key_name, static_cast<int64_t>(v));
      } else {
        return key(key_name, static_cast<uint64_t>(v));
      }
    }

    /// single-line array of scalars: "k = [ 1.0, 2.0 ]"
    template <std::ranges::input_range R>
    toml_writer& key_array(std::string_view key_name, const R& range) {
      toml::array mirror_array;
      std::ostringstream line;
      line << "[";
      bool first = true;
      for (const auto& element : range) {
        line << (first ? " " : ", ");
        first = false;
        line << format_scalar(element, mirror_array);
      }
      line << (first ? "]" : " ]");
      emit_key_line(key_name, line.str());
      mirror_insert(key_name, std::move(mirror_array));
      return *this;
    }

    /// array of inline tables, one element per line:
    ///   k = [
    ///     { a = 1, b = 2 },
    ///   ]
    /// the callback receives an inline_table per element index and fills it via .key(...)
    class inline_table {
     public:
      inline_table& key(std::string_view key_name, bool v);
      inline_table& key(std::string_view key_name, int64_t v);
      inline_table& key(std::string_view key_name, uint64_t v);
      inline_table& key(std::string_view key_name, double v);
      inline_table& key(std::string_view key_name, float v);
      inline_table& key(std::string_view key_name, std::string_view v);
      inline_table& key(std::string_view key_name, const std::string& v) { return key(key_name, std::string_view{ v }); }
      template <std::integral T>
        requires(!std::same_as<T, bool> && !std::same_as<T, int64_t> && !std::same_as<T, uint64_t>)
      inline_table& key(std::string_view key_name, T v) {
        if constexpr (std::is_signed_v<T>) {
          return key(key_name, static_cast<int64_t>(v));
        } else {
          return key(key_name, static_cast<uint64_t>(v));
        }
      }
      template <std::ranges::input_range R>
      inline_table& key_array(std::string_view key_name, const R& range) {
        toml::array mirror_array;
        std::ostringstream text;
        text << "[";
        bool first = true;
        for (const auto& element : range) {
          text << (first ? " " : ", ");
          first = false;
          text << toml_writer::format_scalar(element, mirror_array);
        }
        text << (first ? "]" : " ]");
        append_raw(key_name, text.str());
        mirror.insert(key_name, std::move(mirror_array));
        return *this;
      }

      std::string str(bool space_inside_braces) const;
      toml::table take_mirror() { return std::move(mirror); }

     private:
      friend class toml_writer;
      void append_raw(std::string_view key_name, std::string_view value_text);

      ostd::vector<std::pair<std::string, std::string>> entries;
      toml::table mirror;
    };

    template <typename Fn>
      requires std::invocable<Fn, inline_table&, size_t>
    toml_writer& key_inline_table_array(std::string_view key_name, size_t count, Fn&& fill) {
      toml::array mirror_array;
      std::ostringstream body;
      body << "[\n";
      for (size_t i = 0; i < count; ++i) {
        inline_table element;
        fill(element, i);
        body << "  " << element.str(options.space_inside_inline_braces) << ",\n";
        mirror_array.push_back(element.take_mirror());
      }
      body << "]";
      emit_key_line(key_name, body.str());
      mirror_insert(key_name, std::move(mirror_array));
      return *this;
    }

    /// -- output ------------------------------------------------------------
    /// debug builds re-parse the emitted text and assert equivalence with the mirror tree
    void finalize();
    std::string str();
    bool save(const filepath& file_path);

    const toml::table& document() const { return mirror; }

   private:
    template <typename T>
    static std::string format_scalar(const T& v, toml::array& mirror_array) {
      using no_cvref_t = std::remove_cvref_t<T>;
      if constexpr (std::same_as<no_cvref_t, bool>) {
        mirror_array.push_back(v);
        return v ? "true" : "false";
      } else if constexpr (std::floating_point<no_cvref_t>) {
        /// the mirror stores what the emitted text parses back to, so shortest-form
        ///  float output still passes the debug equivalence check
        const std::string text = format_float(v);
        mirror_array.push_back(parse_emitted_float(text));
        return text;
      } else if constexpr (std::integral<no_cvref_t>) {
        mirror_array.push_back(checked_toml_int(static_cast<uint64_t>(std::max<no_cvref_t>(v, 0)), static_cast<int64_t>(v), std::is_signed_v<no_cvref_t>));
        return std::format("{}", v);
      } else {
        const std::string text{ std::string_view{ v } };
        mirror_array.push_back(text);
        return quote_string(text);
      }
    }

    static std::string format_float(double v);
    static std::string format_float(float v);
    static double parse_emitted_float(const std::string& text);
    static std::string quote_string(std::string_view v);
    static int64_t checked_toml_int(uint64_t unsigned_value, int64_t signed_value, bool is_signed);

    void emit_key_line(std::string_view key_name, std::string_view value_text);
    void mirror_insert(std::string_view key_name, auto&& value) {
      toml::table* target = mirror_current_table();
      OTHER_ASSERT(target != nullptr, "toml_writer mirror table missing for key '{}'", key_name);
      auto [it, inserted] = target->insert(key_name, std::forward<decltype(value)>(value));
      OTHER_ASSERT(inserted, "duplicate toml key '{}' in table '{}'", key_name, current_table_path);
    }
    toml::table* mirror_current_table();
    void flush_pending_blank();

    toml_format_options options;
    std::ostringstream text;
    bool finalized = false;
    bool pending_blank = false;
    bool any_content = false;

    std::string current_table_path = "";      /// "" = document root
    std::set<std::string> declared_tables;    /// explicit [x] headers, dup detection
    std::set<std::string> current_table_keys;

    toml::table mirror;
    toml::table* current_mirror_table = nullptr;
  };

}  // namespace other

#endif  // OTHER_CORE_SERIALIZATION_TOML_WRITER_HPP
