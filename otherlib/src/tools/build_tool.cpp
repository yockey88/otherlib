/**
 * \file tools/build_tool.cpp
 **/
#include "build_tool.hpp"

#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

#include "project/project.hpp"

namespace other {

  // void build_tool::start_build(const project_description& project) {
  //   auto* env = subsystem<scripting_environment>::get();
  //   OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

  //   this->project = project;

  //   build_tool_obj_id = env->create_object("Builder");
  //   if (build_tool_obj_id == -1) {
  //     CORE_LOG_ERROR("Failed to create Builder object to build project {}", project.project_name);
  //     build_tool_obj = nullptr;
  //     curr_build_status = BUILD_STATUS_FAILED;
  //     return;
  //   }

  //   CORE_LOG_DEBUG("Attaching Builder object to scripting environment to build project '{}'", project.project_name);
  //   env->attach_dotnet_object(build_tool_obj_id, "Other.BuildTool");
  //   /// suspend just to let the engine breathe
  //   CORE_LOG_DEBUG("Began building project '{}'", project.project_name);

  //   build_tool_obj = env->get_object(build_tool_obj_id);
  //   if (build_tool_obj == nullptr) {
  //     env->destroy_object(build_tool_obj_id);
  //     CORE_LOG_ERROR("Failed to retrieve Builder object from scripting environment to build project {}", project.project_name);
  //     build_tool_obj = nullptr;
  //     curr_build_status = BUILD_STATUS_FAILED;
  //     return;
  //   }

  //   struct build_args_ {
  //     native_string project_type;
  //     native_string name;
  //     native_string filename;
  //     native_string working_directory;
  //   } args;
  //   /// \todo make project type selectable
  //   args.project_type = "Application";
  //   args.name = project.project_name;
  //   args.filename = project.environment_config.string();
  //   args.working_directory = project.working_directory.string();

  //   CORE_LOG_DEBUG("Invoking CreateProject on Builder object for project '{}'", project.project_name);
  //   build_tool_obj->dotnet_object->invoke<>("CreateProject", args);
  //   curr_build_status = (build_status)build_tool_obj->dotnet_object->invoke<int32_t>("GetBuildStatus");
  // }

  void build_tool::poll_project_build() {
    if (build_tool_obj == nullptr) {
      return;
    }

    curr_build_status = (build_status)build_tool_obj->dotnet_object->invoke<int32_t>("PollProjectBuild");
  }

  void build_tool::finalize_build() {
    if (build_tool_obj == nullptr) {
      return;
    }

    build_tool_obj->dotnet_object->invoke<void>("FinalizeBuild");
    CORE_LOG_DEBUG("Finalized build process.");

    auto* env = subsystem<scripting_environment>::get();
    env->detach_dotnet_object(build_tool_obj_id);
    env->destroy_object(build_tool_obj_id);
    build_tool_obj = nullptr;
    build_tool_obj_id = -1;
  }

  bool build_tool::finished_project_generation() const {
    return curr_build_status == BUILD_STATUS_SUCCESS || curr_build_status == BUILD_STATUS_FAILED;
  }

  build_tool::build_status build_tool::get_build_status() {
    if (build_tool_obj == nullptr) {
      return curr_build_status;
    }
    curr_build_status = (build_status)build_tool_obj->dotnet_object->invoke<int32_t>("GetBuildStatus");
    return curr_build_status;
  }

}  // namespace other