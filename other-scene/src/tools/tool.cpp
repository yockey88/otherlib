/**
 * \file tools/tool.cpp
 **/
#include "tools/tool.hpp"

#include "script/scripting_environment.hpp"

namespace other {

  tool::tool(const std::string& type_name) : type_name(type_name) {
    initialize();
  }

  tool::~tool() {
    shutdown();
  }

  void tool::initialize() {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment subsystem is not available.");

    build_tool_obj_id = env->create_object(type_name);
    if (build_tool_obj_id == -1) {
      CORE_LOG_ERROR("Failed to create script object for tool of type '{}'", type_name);
      return;
    }

    env->attach_dotnet_object(build_tool_obj_id, type_name);

    build_tool_obj = env->get_object(build_tool_obj_id);
    OTHER_ASSERT(build_tool_obj != nullptr, "Failed to get script object for tool of type '{}'", type_name);
    OTHER_ASSERT(build_tool_obj->dotnet_object != nullptr, "Script object for tool of type '{}' does not have a .NET object attached.", type_name);
  }

  void tool::shutdown() {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment subsystem is not available.");

    CORE_LOG_DEBUG("Shutting down tool of type '{}'", type_name);
    if (build_tool_obj_id != -1) {
      env->destroy_object(build_tool_obj_id);
      build_tool_obj_id = -1;
      build_tool_obj = nullptr;
    }
  }

}  // namespace other