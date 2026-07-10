/**
 * \file tools/build_tool.hpp
 **/
#ifndef OTHERLIB_TOOLS_BUILD_TOOL_HPP
#define OTHERLIB_TOOLS_BUILD_TOOL_HPP

#include "core/defines.hpp"

#include "script/script_object.hpp"

#include "project/project.hpp"

namespace other {

  class build_tool {
   public:
    enum build_status : int32_t {
      BUILD_STATUS_NOT_STARTED = 0,
      GENERATING_BUILD_SYSTEM,
      CURRENTLY_BUILDING,
      BUILD_STATUS_SUCCESS,
      BUILD_STATUS_FAILED,

      NUM_BUILD_STATUSES,
      INVALID_BUILD_STATUS = NUM_BUILD_STATUSES,
    };

    build_tool() = default;
    ~build_tool() = default;

    /// returns json blob with build results and info
    // void start_build(const project_description& project);
    void poll_project_build();
    void finalize_build();

    bool finished_project_generation() const;

    /// updates and retrieves current build status
    build_status get_build_status();

    /*
    std::string name = "NewProject";
    filepath working_directory = "${CWD}";

    filepath output_directory = "${CWD}/build";
    filepath exe_name = "${CWD}/OtherApp.exe";

    ostd::vector<std::string> configurations = { "Debug", "Release" };
    size_t active_configuration = 0;

    ostd::vector<std::string> cmd_args = {};

    std::string version = "0.1.0";
    std::string description = "An Other project.";
    std::string author = "Author Name";

    std::string license = "MIT";
    */

   private:
    integer_t build_tool_obj_id = -1;
    script_object* build_tool_obj = nullptr;

    build_status curr_build_status = BUILD_STATUS_NOT_STARTED;
    // project_description project = {};

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