/**
 * \file project/project.hpp
 **/
#ifndef OTHERLIB_PROJECT_PROJECT_HPP
#define OTHERLIB_PROJECT_PROJECT_HPP

#include "core/defines.hpp"

namespace other {

  struct project_description {
    enum type : uint16_t {
      APPLICATION = 0,
      MODULE,

      NUM_PROJECT_TYPES,
      INVALID_PROJECT_TYPE = NUM_PROJECT_TYPES,
    };
    type project_type = APPLICATION;
    std::string project_name = "NewProject";

    filepath environment_config = "${project-directory}/${project-name}.toml";
    filepath working_directory = "${project-directory}";
    filepath output_directory = "${project-directory}/build";
    filepath exe_name = "${project-directory}/${project-name}.exe";

    std::vector<std::string> configurations = { "Debug", "Release" };

    size_t active_configuration = 0;
    std::vector<std::string> cmd_args = {};

    std::string version = "0.1.0";
    std::string description = "An Other project.";
    std::string author = "Author Name";

    std::string license = "MIT";
  };

}  // namespace other

#endif  // OTHERLIB_PROJECT_PROJECT_HPP