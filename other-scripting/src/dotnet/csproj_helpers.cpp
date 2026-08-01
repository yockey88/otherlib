/**
 * \file dotnet/csproj_helpers.cpp
 **/
#include "dotnet/csproj_helpers.hpp"

namespace other {
  namespace cs_xml {

    std::string_view item::include() const {
      OTHER_ASSERT(!remove, "include() on a Remove item");
      return entry;
    }

    std::string item::metadata(const char* element_name) const {
      OTHER_ASSERT(element != nullptr, "metadata() on a default-constructed item");
      const std::string_view text = xml::child_text(*element, element_name);
      return text.empty() ? std::string{} : substitute_properties(text, dotnet_config);
    }

  }  // namespace cs_xml

  std::string substitute_properties(std::string_view text, std::string_view dotnet_config) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
      const size_t open = text.find("$(", i);
      if (open == std::string_view::npos) {
        out += text.substr(i);
        break;
      }
      out += text.substr(i, open - i);
      const size_t close = text.find(')', open + 2);
      OTHER_ASSERT(close != std::string_view::npos, "unterminated property in '{}'", text);
      const std::string_view name = text.substr(open + 2, close - open - 2);

      OTHER_ASSERT(name == "Configuration", "unsupported property $({}) in '{}'", name, text);
      out += dotnet_config;
      i = close + 1;
    }
    return out;
  }

  bool evaluate_condition(std::string_view condition, std::string_view dotnet_config) {
    std::string_view s = detail::trim(condition);

    const auto take_quoted = [&]() -> std::string {
      OTHER_ASSERT(!s.empty() && s.front() == '\'', "unsupported Condition '{}'", condition);
      s.remove_prefix(1);

      const size_t end = s.find('\'');
      OTHER_ASSERT(end != std::string_view::npos, "unsupported Condition '{}'", condition);

      const std::string value = substitute_properties(s.substr(0, end), dotnet_config);
      s.remove_prefix(end + 1);
      s = detail::trim(s);
      return value;
    };

    const std::string lhs = take_quoted();
    OTHER_ASSERT(s.size() >= 2 && (s.starts_with("==") || s.starts_with("!=")), "unsupported Condition '{}'", condition);

    const bool negate = s.starts_with("!=");
    s.remove_prefix(2);
    s = detail::trim(s);

    const std::string rhs = take_quoted();
    OTHER_ASSERT(s.empty(), "trailing text in Condition '{}'", condition);

    const bool equal = detail::equals_case_insensitive(lhs, rhs);
    return negate ? !equal : equal;
  }

}  // namespace other