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
    std::string name = "NewProject";
    std::string override_file_name = "";
    filepath working_directory = "${CWD}";

    filepath output_directory = "${CWD}/build";
    filepath exe_name = "${CWD}/OtherApp.exe";

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