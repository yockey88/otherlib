/**
 * \file cli/environment.hpp
 **/
#ifndef OTHER_CLI_ENVIRONMENT_HPP
#define OTHER_CLI_ENVIRONMENT_HPP

#include <array>
#include <string_view>

#include "core/defines.hpp"

namespace other {
  namespace cli {

    /// resolved locations of an Other Environment (source tree or installed SDK); tools use
    ///  these to find the editor, the engine assemblies, and project templates
    struct environment_paths {
      filepath root = "";
      filepath editor_config = "";
      filepath templates_dir = "";

      bool in_source_tree = false;
      bool found = false;

      /// source tree only; the installed SDK does not ship the editor yet, so this
      ///  returns an empty path for installed environments
      filepath editor_executable(std::string_view build_config) const;

      /// engine C# bindings assembly for the given build config; installed SDKs ship a
      ///  single assembly directory, source trees split by Debug/Release
      filepath othercs_assembly(std::string_view build_config) const;
    };

    constexpr inline std::array<std::string_view, 4> kBuildConfigs = { "Debug", "Release", "Profile", "ProfileD" };

    bool is_valid_build_config(std::string_view build_config);

    /// discovery order: explicit root -> OTHER_ENVIRONMENT_ROOT env var -> walk up from this
    ///  executable -> walk up from the working directory -> default install location;
    ///  returns found == false when nothing matches (an explicit root is never walked)
    environment_paths locate_environment(const opt<filepath>& explicit_root = std::nullopt);

    /// probe order for per-config engine binaries, starting with the config this binary
    ///  was compiled as (other::get_environment_build_config_string())
    std::array<std::string_view, 4> build_config_probe_order();

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_ENVIRONMENT_HPP
