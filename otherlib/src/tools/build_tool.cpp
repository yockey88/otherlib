/**
 * \file tools/build_tool.cpp
 **/
#include "build_tool.hpp"

#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

#include "project/project.hpp"

namespace other {

  void build_tool::start_build(const project_description& project) {
    this->project = project;

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment subsystem is not initialized");

    build_tool_obj_id = script_env->create_object("Builder");
    OTHER_ASSERT(build_tool_obj_id != -1, "Failed to create Builder object in scripting environment");

    script_env->attach_dotnet_object(build_tool_obj_id, "Other.BuildTool");
    script_object* builder_obj = script_env->get_object(build_tool_obj_id);

    struct build_args {
      native_string project_type;
      native_string name;
      native_string override_file_name;
      native_string working_directory;
    };

    build_args args = build_args{
      .name = native_string::new_str(project.name),
      .working_directory = native_string::new_str(project.working_directory.string()),
    };

    switch (project.project_type) {
      case project_description::APPLICATION: args.project_type = native_string::new_str("Application"); break;
      case project_description::MODULE: args.project_type = native_string::new_str("Module"); break;
      default: args.project_type = native_string::new_str("Unknown"); break;
    }
    if (project.override_file_name != "") {
      args.override_file_name = native_string::new_str(project.override_file_name);
    }

    builder_obj->dotnet_object->invoke("StartBuild", args);
  }

}  // namespace other