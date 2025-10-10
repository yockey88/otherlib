/**
 * \file tools/build_tool.hpp
 **/
#ifndef OTHERLIB_TOOLS_BUILD_TOOL_HPP
#define OTHERLIB_TOOLS_BUILD_TOOL_HPP

#include "core/defines.hpp"

#include "project/project.hpp"


namespace other {

  class build_tool {
   public:
    build_tool() = default;
    ~build_tool() = default;

    void start_build(const project_description& project);
    // void finalize_build();

    /*
    std::string name = "NewProject";
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
    */

   private:
    integer_t build_tool_obj_id = -1;

    project_description project = {};

    enum build_phase {
      VALIDATE_GENERATE_FILES = 0,
      PRE_BUILD_STEPS,
      BUILD,
      POST_BUILD_STEPS,
      CLEANUP,

      NUM_BUILD_PHASES,
      INVALID_BUILD_PHASE = NUM_BUILD_PHASES,
    };
  };

}  // namespace other

#endif  // OTHERLIB_TOOLS_BUILD_TOOL_HPP