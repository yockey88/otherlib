/**
 * \file dotnet/csproj_helpers.hpp
 */
#ifndef OTHER_SCRIPTING_DOTNET_CSPROJ_HELPERS_HPP
#define OTHER_SCRIPTING_DOTNET_CSPROJ_HELPERS_HPP

#include "file/xml_helpers.hpp"

namespace other {
  namespace cs_xml {

    struct item {
      const tinyxml2::XMLElement* element = nullptr;
      std::string entry;
      bool remove = false;
      std::string_view dotnet_config;

      bool is_remove() const { return remove; }
      std::string_view pattern() const { return entry; }
      filepath hint_path() const { return filepath(metadata("HintPath")); }

      std::string_view include() const;
      std::string metadata(const char* element_name) const;
    };

  }  // namespace cs_xml

  constexpr std::string_view dotnet_config_for(std::string_view build_config) {
    if (build_config == "Debug" || build_config == "ProfileD") {
      return "Debug";
    }
    if (build_config == "Release" || build_config == "Profile") {
      return "Release";
    }

    OTHER_ASSERT(false, "unknown build configuration '{}'", build_config);
    return "Debug";
  }

  std::string substitute_properties(std::string_view text, std::string_view dotnet_config);
  bool evaluate_condition(std::string_view condition, std::string_view dotnet_config);

  template <typename T>
  T csproj_property_or(const tinyxml2::XMLDocument& doc, std::string_view property_name, T default_value) {
    const tinyxml2::XMLElement* project = doc.FirstChildElement("Project");
    OTHER_ASSERT(project != nullptr, "csproj has no <Project> root");

    std::string prop{ property_name };
    for (const tinyxml2::XMLElement& group : xml::children(*project, "PropertyGroup")) {
      if (xml::attribute(group, "Condition").has_value()) {
        continue;
      }
      if (const tinyxml2::XMLElement* p = group.FirstChildElement(prop.data()); p != nullptr) {
        return xml::parse_property<T>(xml::text_of(*p), property_name);
      }
    }

    for (const tinyxml2::XMLElement& group : xml::children(*project, "PropertyGroup")) {
      if (xml::attribute(group, "Condition").has_value() && group.FirstChildElement(prop.data()) != nullptr) {
        OTHER_ASSERT(false, "property <{}> exists only in a conditional PropertyGroup — unsupported", property_name);
      }
    }

    return default_value;
  }

  void for_each_item(const tinyxml2::XMLDocument& doc, const char* item_name, std::string_view dotnet_config, std::invocable<const cs_xml::item&> auto&& fn) {
    const tinyxml2::XMLElement* project = doc.FirstChildElement("Project");
    OTHER_ASSERT(project != nullptr, "csproj has no <Project> root");

    for (const tinyxml2::XMLElement& group : xml::children(*project, "ItemGroup")) {
      if (const opt<std::string_view> cond = xml::attribute(group, "Condition");
          cond.has_value() && !evaluate_condition(*cond, dotnet_config)) {
        continue;
      }

      for (const tinyxml2::XMLElement& el : xml::children(group, item_name)) {
        const opt<std::string_view> include = xml::attribute(el, "Include");
        const opt<std::string_view> remove = xml::attribute(el, "Remove");
        OTHER_ASSERT(!xml::attribute(el, "Update").has_value(), "<{} Update> is unsupported", item_name);
        OTHER_ASSERT(include.has_value() != remove.has_value(), "<{}> must carry exactly one of Include/Remove", item_name);

        const std::string substituted = substitute_properties(include.has_value() ? *include : *remove, dotnet_config);
        for (const std::string_view raw : detail::split(substituted, ';')) {
          const std::string_view entry = detail::trim(raw);
          if (entry.empty()) {
            continue;
          }

          std::string normalized{ entry };
          std::ranges::replace(normalized, '\\', '/');
          fn(cs_xml::item{ &el, std::move(normalized), remove.has_value(), dotnet_config });
        }
      }
    }
  }

  static void validate_supported_csproj_shape(const tinyxml2::XMLElement& project,
                                              const filepath& csproj) {
    /// arbitrary build logic - unresolvable statically
    for (const char* forbidden : { "Import", "Target", "UsingTask", "Choose" }) {
      OTHER_ASSERT(project.FirstChildElement(forbidden) == nullptr, "csproj '{}' using <{}> is unsupported", csproj.string(), forbidden);
    }

    /// wildcard ProjectReference - never generated, ambiguity not worth supporting.
    for (const tinyxml2::XMLElement& group : xml::children(project, "ItemGroup")) {
      for (const tinyxml2::XMLElement& reference : xml::children(group, "ProjectReference")) {
        if (const opt<std::string_view> include = xml::attribute(reference, "Include"); include.has_value()) {
          OTHER_ASSERT(include->find_first_of("*?") == std::string_view::npos, "wildcard ProjectReference '{}' in '{}' is unsupported", *include, csproj.string());
        }
      }
    }
  }

  static void warn_on_directory_build_files(const filepath& csproj) {
    for (filepath dir = dir_of(csproj); !dir.empty(); dir = dir.parent_path()) {
      for (const std::string_view name : { "Directory.Build.props", "Directory.Build.targets" }) {
        if (std::filesystem::exists(dir / name)) {
          CORE_LOG_WARN("'{}' found at '{}' will be ignored", name, dir.string());
        }
      }

      if (!is_under_any_project_mount(dir)) {
        break;
      }
    }
  }

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_CSPROJ_HELPERS_HPP