/**
 * \file file/xml_helpers.hpp
 **/
#ifndef OTHER_CORE_FILE_XML_HELPERS_HPP
#define OTHER_CORE_FILE_XML_HELPERS_HPP

#include <tinyxml2/tinyxml2.h>

#include "file/path_helpers.hpp"

namespace other {
  namespace xml {

    template <typename>
    inline constexpr bool always_false_v = false;

    class element_range {
     public:
      class iterator {
       public:
        iterator(const tinyxml2::XMLElement* el, const char* name)
            : el(el), name(name) {}
        const tinyxml2::XMLElement& operator*() const {
          OTHER_ASSERT(el != nullptr, "dereferencing end element iterator");
          return *el;
        }
        iterator& operator++() {
          el = el->NextSiblingElement(name);
          return *this;
        }
        bool operator==(const iterator& other) const { return el == other.el; }

       private:
        const tinyxml2::XMLElement* el;
        const char* name;
      };

      element_range(const tinyxml2::XMLElement& parent, const char* name)
          : first(parent.FirstChildElement(name)), name(name) {}

      iterator begin() const { return { first, name }; }
      iterator end() const { return { nullptr, name }; }
      bool empty() const { return first == nullptr; }

     private:
      const tinyxml2::XMLElement* first;
      const char* name;
    };

    template <typename T>
    T parse_property(std::string_view text, std::string_view property_name) {
      static_assert(always_false_v<T>, "no csproj property parser for this type — add a specialization");
    }

    template <>
    inline std::string parse_property<std::string>(std::string_view text, std::string_view) {
      return std::string{ text };
    }

    template <>
    inline bool parse_property<bool>(std::string_view text, std::string_view property_name) {
      if (detail::equals_case_insensitive(text, "true")) {
        return true;
      }

      if (detail::equals_case_insensitive(text, "false")) {
        return false;
      }

      OTHER_ASSERT(false, "unparseable bool '{}' in <{}>", text, property_name);
      return false;
    }

    template <>
    inline int64_t parse_property<int64_t>(std::string_view text, std::string_view property_name) {
      int64_t value = 0;
      const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
      OTHER_ASSERT(ec == std::errc{} && ptr == text.data() + text.size(), "unparseable integer '{}' in <{}>", text, property_name);
      return value;
    }

    inline element_range children(const tinyxml2::XMLElement& parent, const char* name) {
      return element_range{ parent, name };
    }

    inline opt<std::string_view> attribute(const tinyxml2::XMLElement& el, const char* name) {
      const char* value = el.Attribute(name);
      return value != nullptr ?
        opt<std::string_view>{ std::string_view{ value } } :
        std::nullopt;
    }

    inline std::string_view text_of(const tinyxml2::XMLElement& el) {
      const char* text = el.GetText();
      return text != nullptr ?
        std::string_view{ text } :
        std::string_view{};
    }

    inline std::string_view child_text(const tinyxml2::XMLElement& el, const char* child_name) {
      const tinyxml2::XMLElement* child = el.FirstChildElement(child_name);
      return child != nullptr ? text_of(*child) : std::string_view{};
    }

    inline const tinyxml2::XMLElement& load_project_root(tinyxml2::XMLDocument& doc, const filepath& file) {
      OTHER_ASSERT(doc.LoadFile(file.string().c_str()) == tinyxml2::XML_SUCCESS,
                   "unparseable xml '{}': {}", file.string(), doc.ErrorStr());
      const tinyxml2::XMLElement* root = doc.FirstChildElement("Project");
      OTHER_ASSERT(root != nullptr, "'{}' has no <Project> root", file.string());
      return *root;
    }

  }  // namespace xml
}  // namespace other

#endif  // OTHER_CORE_FILE_XML_HELPERS_HPP