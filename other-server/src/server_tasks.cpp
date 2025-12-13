/**
 * \file server_tasks.cpp
 **/
#include "server_tasks.hpp"

#include "tools/build_tool.hpp"

namespace other {

  task build_project(const project_creator::project_context& context, json::json& project_cache_path, event_system& events) {
    /// have to copy context since coroutine may outlive project_creator ui-page
    project_creator::project_context ctx = context;

    build_tool builder = {};
    project_description project_desc = {};
    project_desc.project_type = project_description::APPLICATION;
    project_desc.project_name = ctx.project_name;
    project_desc.environment_config = ctx.project_path;
    project_desc.working_directory = ctx.working_directory;
    CORE_LOG_DEBUG("Starting build for project '{}' at path '{}' with working directory '{}'", ctx.project_name, ctx.project_path.string(), ctx.working_directory.string());
    builder.start_build(project_desc);
    while (true) {
      builder.poll_project_build();
      if (builder.finished_project_generation()) {
        CORE_LOG_DEBUG("Build process for project '{}' finished.", ctx.project_name);
        break;
      }
      co_await task::awaiter{};
    }

    builder.finalize_build();
    if (builder.get_build_status() == build_tool::BUILD_STATUS_FAILED) {
      CORE_LOG_ERROR("Build for project '{}' failed.", ctx.project_name);
      co_return;
    } else {
      CORE_LOG_DEBUG("Build for project '{}' completed successfully.", ctx.project_name);
    }

    /// update project cache
    json::json new_project_entry;
    new_project_entry["name"] = ctx.project_name;
    new_project_entry["project-file"] = ctx.project_path.string();
    new_project_entry["working-directory"] = ctx.working_directory.string();
    new_project_entry["configurations"] = std::vector<std::string>{ "Debug", "Release" };
    new_project_entry["build"] = {
      { "type", "other-application" },
      { "output-folder", (filepath(ctx.working_directory) / "build/${configuration}").string() },
      { "executable", (filepath(ctx.working_directory) / "build/${configuration}" / (ctx.project_name + ".exe")).string() },
      { "args", std::vector<std::string>{} }
    };
    project_cache_path["projects"].push_back(new_project_entry);
    /// rewrite entire cache back to file, we could optimize this later
    /// we might just want to defer this to do on shutdown of server, but this also could lead to data loss if the server crashes
    {
      filepath project_cache_file = get_app_data_folder("OtherEngine/OtherServer") / filepath("project_cache.json");
      std::ofstream file(project_cache_file, std::ios::trunc);
      /// we want pretty json :)
      file << project_cache_path.dump(2);
      file.close();
    }

    CORE_LOG_DEBUG("Project cache updated with new project '{}', returning to projects page", ctx.project_name);
    events.trigger_event("goto-home-page");
    co_return;
  }

}  // namespace other