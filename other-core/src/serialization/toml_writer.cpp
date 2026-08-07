/**
 * \file serialization/toml_writer.cpp
 **/
#include "serialization/toml_writer.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {
  namespace {

    ostd::vector<std::string> split_dotted_path(std::string_view dotted_path) {
      OTHER_ASSERT(!dotted_path.empty(), "empty toml table path");
      OTHER_ASSERT(dotted_path.front() != '.' && dotted_path.back() != '.', "malformed toml table path '{}'", dotted_path);
      ostd::vector<std::string> segments;
      size_t start = 0;
      while (start <= dotted_path.size()) {
        const size_t dot = dotted_path.find('.', start);
        const std::string_view segment = dotted_path.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start);
        OTHER_ASSERT(!segment.empty(), "malformed toml table path '{}'", dotted_path);
        segments.emplace_back(segment);
        if (dot == std::string_view::npos) {
          break;
        }
        start = dot + 1;
      }
      return segments;
    }

    /// bare keys need no quoting; anything else gets basic-string quoting
    bool is_bare_key(std::string_view key) {
      if (key.empty()) {
        return false;
      }
      for (const char c : key) {
        const bool bare = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!bare) {
          return false;
        }
      }
      return true;
    }

  }  // namespace

  std::string toml_writer::format_float(double v) {
    if (std::isnan(v)) {
      return "nan";
    }
    if (std::isinf(v)) {
      return v < 0.0 ? "-inf" : "inf";
    }
    std::string text = std::format("{}", v);
    /// toml floats require a fractional part or exponent; "{}" gives shortest round-trip
    ///  which drops ".0" for whole numbers
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos && text.find('E') == std::string::npos) {
      text += ".0";
    }
    return text;
  }

  std::string toml_writer::format_float(float v) {
    if (std::isnan(v)) {
      return "nan";
    }
    if (std::isinf(v)) {
      return v < 0.f ? "-inf" : "inf";
    }
    /// formatting the float itself yields its shortest round-trip form ("0.2"), not the
    ///  exact expansion of the widened double ("0.20000000298...")
    std::string text = std::format("{}", v);
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos && text.find('E') == std::string::npos) {
      text += ".0";
    }
    return text;
  }

  double toml_writer::parse_emitted_float(const std::string& text) {
    return std::strtod(text.c_str(), nullptr);
  }

  std::string toml_writer::quote_string(std::string_view v) {
    std::string out;
    out.reserve(v.size() + 2);
    out += '"';
    for (const char c : v) {
      switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '\r': out += "\\r"; break;
        default:
          if (static_cast<unsigned char>(c) < 0x20) {
            out += std::format("\\u{:04X}", static_cast<uint32_t>(static_cast<unsigned char>(c)));
          } else {
            out += c;
          }
          break;
      }
    }
    out += '"';
    return out;
  }

  int64_t toml_writer::checked_toml_int(uint64_t unsigned_value, int64_t signed_value, bool is_signed) {
    if (!is_signed) {
      OTHER_ASSERT(unsigned_value <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
                   "unsigned value {} does not fit in a toml integer", unsigned_value);
    }
    return signed_value;
  }

  toml_writer::toml_writer(toml_format_options options)
      : options(options) {}

  toml_writer& toml_writer::comment(std::string_view text_line) {
    OTHER_ASSERT(!finalized, "toml_writer used after finalize");
    flush_pending_blank();
    text << "# " << text_line << "\n";
    any_content = true;
    return *this;
  }

  toml_writer& toml_writer::blank() {
    OTHER_ASSERT(!finalized, "toml_writer used after finalize");
    pending_blank = any_content;
    return *this;
  }

  toml::table* toml_writer::mirror_current_table() {
    if (current_table_path.empty()) {
      return &mirror;
    }
    toml::node* node = &mirror;
    for (const std::string& segment : split_dotted_path(current_table_path)) {
      toml::table* as_table = node->as_table();
      OTHER_ASSERT(as_table != nullptr, "toml_writer mirror path '{}' broken at '{}'", current_table_path, segment);
      toml::node* child = as_table->get(segment);
      OTHER_ASSERT(child != nullptr, "toml_writer mirror path '{}' missing '{}'", current_table_path, segment);
      if (toml::array* as_array = child->as_array(); as_array != nullptr) {
        OTHER_ASSERT(!as_array->empty(), "toml_writer mirror array '{}' empty", segment);
        node = &as_array->back();
      } else {
        node = child;
      }
    }
    toml::table* result = node->as_table();
    OTHER_ASSERT(result != nullptr, "toml_writer mirror current node is not a table ('{}')", current_table_path);
    return result;
  }

  toml_writer& toml_writer::table(std::string_view dotted_path) {
    OTHER_ASSERT(!finalized, "toml_writer used after finalize");
    PROFILE_SECTION("toml_writer::table");
    const ostd::vector<std::string> segments = split_dotted_path(dotted_path);

    /// walk the mirror, descending through the last element of any table-array segment;
    ///  intermediate tables are created implicitly, the final table must not already be
    ///  an explicit header (per-array-element, so "[objects.x]" is fine once per [[objects]])
    toml::table* node = &mirror;
    std::string decorated_path;
    for (size_t i = 0; i < segments.size(); ++i) {
      const std::string& segment = segments[i];
      toml::node* child = node->get(segment);
      if (child == nullptr) {
        auto [it, inserted] = node->insert(segment, toml::table{});
        OTHER_ASSERT(inserted, "toml_writer failed to insert mirror table '{}'", segment);
        child = &it->second;
      }
      if (toml::array* as_array = child->as_array(); as_array != nullptr) {
        OTHER_ASSERT(!as_array->empty() && as_array->back().is_table(), "toml table path '{}' crosses non-table-array '{}'", dotted_path, segment);
        decorated_path += std::format("{}@{}.", segment, as_array->size() - 1);
        node = as_array->back().as_table();
      } else {
        OTHER_ASSERT(child->is_table(), "toml table path '{}' crosses non-table '{}'", dotted_path, segment);
        decorated_path += segment + ".";
        node = child->as_table();
      }
    }

    const auto [it, inserted] = declared_tables.insert(decorated_path);
    OTHER_ASSERT(inserted, "duplicate toml table header '[{}]'", dotted_path);

    flush_pending_blank();
    text << "[" << dotted_path << "]\n";
    any_content = true;

    current_table_path = dotted_path;
    current_table_keys.clear();
    return *this;
  }

  toml_writer& toml_writer::table_array(std::string_view dotted_path) {
    OTHER_ASSERT(!finalized, "toml_writer used after finalize");
    PROFILE_SECTION("toml_writer::table_array");
    const ostd::vector<std::string> segments = split_dotted_path(dotted_path);

    toml::table* node = &mirror;
    for (size_t i = 0; i + 1 < segments.size(); ++i) {
      const std::string& segment = segments[i];
      toml::node* child = node->get(segment);
      if (child == nullptr) {
        auto [it, inserted] = node->insert(segment, toml::table{});
        OTHER_ASSERT(inserted, "toml_writer failed to insert mirror table '{}'", segment);
        child = &it->second;
      }
      if (toml::array* as_array = child->as_array(); as_array != nullptr) {
        OTHER_ASSERT(!as_array->empty() && as_array->back().is_table(), "toml table-array path '{}' crosses non-table-array '{}'", dotted_path, segment);
        node = as_array->back().as_table();
      } else {
        OTHER_ASSERT(child->is_table(), "toml table-array path '{}' crosses non-table '{}'", dotted_path, segment);
        node = child->as_table();
      }
    }

    const std::string& last = segments.back();
    toml::node* child = node->get(last);
    toml::array* target_array = nullptr;
    if (child == nullptr) {
      auto [it, inserted] = node->insert(last, toml::array{});
      OTHER_ASSERT(inserted, "toml_writer failed to insert mirror table-array '{}'", last);
      target_array = it->second.as_array();
    } else {
      target_array = child->as_array();
      OTHER_ASSERT(target_array != nullptr, "toml table-array path '{}' collides with a non-array", dotted_path);
    }
    target_array->push_back(toml::table{});

    flush_pending_blank();
    text << "[[" << dotted_path << "]]\n";
    any_content = true;

    current_table_path = dotted_path;
    current_table_keys.clear();
    return *this;
  }

  void toml_writer::emit_key_line(std::string_view key_name, std::string_view value_text) {
    OTHER_ASSERT(!finalized, "toml_writer used after finalize");
    OTHER_ASSERT(!key_name.empty(), "empty toml key");
    const auto [it, inserted] = current_table_keys.insert(std::string{ key_name });
    OTHER_ASSERT(inserted, "duplicate toml key '{}' in table '{}'", key_name, current_table_path.empty() ? "<root>" : current_table_path);

    flush_pending_blank();
    if (is_bare_key(key_name)) {
      text << key_name << " = " << value_text << "\n";
    } else {
      text << quote_string(key_name) << " = " << value_text << "\n";
    }
    any_content = true;
  }

  toml_writer& toml_writer::key(std::string_view key_name, bool v) {
    emit_key_line(key_name, v ? "true" : "false");
    mirror_insert(key_name, v);
    return *this;
  }

  toml_writer& toml_writer::key(std::string_view key_name, int64_t v) {
    emit_key_line(key_name, std::format("{}", v));
    mirror_insert(key_name, v);
    return *this;
  }

  toml_writer& toml_writer::key(std::string_view key_name, uint64_t v) {
    const int64_t checked = checked_toml_int(v, static_cast<int64_t>(v), false);
    emit_key_line(key_name, std::format("{}", v));
    mirror_insert(key_name, checked);
    return *this;
  }

  toml_writer& toml_writer::key(std::string_view key_name, double v) {
    emit_key_line(key_name, format_float(v));
    mirror_insert(key_name, v);
    return *this;
  }

  toml_writer& toml_writer::key(std::string_view key_name, float v) {
    const std::string text = format_float(v);
    emit_key_line(key_name, text);
    mirror_insert(key_name, parse_emitted_float(text));
    return *this;
  }

  toml_writer& toml_writer::key(std::string_view key_name, std::string_view v) {
    emit_key_line(key_name, quote_string(v));
    mirror_insert(key_name, std::string{ v });
    return *this;
  }

  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, bool v) {
    append_raw(key_name, v ? "true" : "false");
    mirror.insert(key_name, v);
    return *this;
  }
  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, int64_t v) {
    append_raw(key_name, std::format("{}", v));
    mirror.insert(key_name, v);
    return *this;
  }
  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, uint64_t v) {
    const int64_t checked = toml_writer::checked_toml_int(v, static_cast<int64_t>(v), false);
    append_raw(key_name, std::format("{}", v));
    mirror.insert(key_name, checked);
    return *this;
  }
  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, double v) {
    append_raw(key_name, toml_writer::format_float(v));
    mirror.insert(key_name, v);
    return *this;
  }
  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, float v) {
    const std::string text = toml_writer::format_float(v);
    append_raw(key_name, text);
    mirror.insert(key_name, toml_writer::parse_emitted_float(text));
    return *this;
  }
  toml_writer::inline_table& toml_writer::inline_table::key(std::string_view key_name, std::string_view v) {
    append_raw(key_name, toml_writer::quote_string(v));
    mirror.insert(key_name, std::string{ v });
    return *this;
  }

  void toml_writer::inline_table::append_raw(std::string_view key_name, std::string_view value_text) {
    for (const auto& [existing, _] : entries) {
      OTHER_ASSERT(existing != key_name, "duplicate toml key '{}' in inline table", key_name);
    }
    entries.emplace_back(std::string{ key_name }, std::string{ value_text });
  }

  std::string toml_writer::inline_table::str(bool space_inside_braces) const {
    std::ostringstream out;
    out << (space_inside_braces ? "{ " : "{");
    for (size_t i = 0; i < entries.size(); ++i) {
      if (i > 0) {
        out << ", ";
      }
      const auto& [k, v] = entries[i];
      if (is_bare_key(k)) {
        out << k << " = " << v;
      } else {
        out << toml_writer::quote_string(k) << " = " << v;
      }
    }
    out << (space_inside_braces ? " }" : "}");
    return out.str();
  }

  void toml_writer::flush_pending_blank() {
    if (pending_blank) {
      text << "\n";
      pending_blank = false;
    }
  }

  void toml_writer::finalize() {
    PROFILE_SECTION("toml_writer::finalize");
    if (finalized) {
      return;
    }
    finalized = true;
#ifdef OTHER_DEBUG_BUILD
    try {
      const toml::table parsed = toml::parse(text.str());
      OTHER_ASSERT(parsed == mirror, "toml_writer emitted text diverges from its mirror document");
    } catch (const toml::parse_error& e) {
      OTHER_ASSERT(false, "toml_writer emitted unparseable toml: {}", std::string{ e.description() });
    }
#endif
  }

  std::string toml_writer::str() {
    finalize();
    return text.str();
  }

  bool toml_writer::save(const filepath& file_path) {
    PROFILE_SECTION("toml_writer::save");
    finalize();
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      CORE_LOG_ERROR("toml_writer failed to open '{}' for writing", file_path.string());
      return false;
    }
    const std::string contents = text.str();
    out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    return out.good();
  }

}  // namespace other
