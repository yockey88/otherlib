/**
 * \file server.cpp
 **/
#include "server.hpp"

#include <filesystem>

#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "thread/message.hpp"

#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "rendering-pipelines/empty_pipeline.hpp"

#include "server_tasks.hpp"

namespace other {

  void server::on_initialize(const command_line& cmd) {
    filepath app_folder = get_project_cache();

    std::ifstream file(app_folder);
    if (file.is_open()) {
      file >> project_cache;
      file.close();
    } else {
      CORE_LOG_WARN("Failed to open project cache file at {}", app_folder.string());
    }

    /// create event system
    get_event_system()->register_event("open-project");
    get_event_system()->add_listener("open-project", [this](const value& data) {
      /// \todo open project
      CORE_LOG_DEBUG("Received request to open project");
      std::string name = data;

      filepath project_path;
      filepath working_dir;

      auto projects = project_cache["projects"];
      json::json project_entry;
      for (const auto& p : projects.items()) {
        if (p.value().contains("name") && p.value()["name"].get<std::string>() == name) {
          project_entry = p.value();
          break;
        }
      }

      if (project_entry.is_null()) {
        CORE_LOG_ERROR("Project '{}' not found in project cache", name);
        return;
      }

      std::string file = project_entry.contains("project-file") ? project_entry["project-file"].get<std::string>() : "";

      CORE_LOG_DEBUG("Opening project '{}' at path '{}' with working directory '{}'", name, file, project_entry.at("working-directory").get<std::string>());
      validate_project_and_launch(project_entry);
    });

    get_event_system()->register_event("finalize-project");
    get_event_system()->add_listener("finalize-project", [this](const value& data) {
      post_coroutine(build_project(data, project_cache, *get_event_system()));
    });
  }

  void server::on_initialize_rendering(scope<renderer>& renderer_ptr) {
    renderer_ptr->add_pipeline<empty_pipeline>("UI Pipeline");
  }

  void server::on_initialize_ui(scope<driver_ui>& ui_ptr) {
    this->ui_ptr = make_scope<server_ui>(get_event_system(), project_cache);
  }

  void server::on_update() {
  }

  void server::on_ui_render() {
    ui_ptr->render();
  }

  void server::on_shutdown() {
    CORE_LOG_INFO("Server shutdown complete.");
  }

  void server::on_shutdown_rendering() {
    get_renderer_instance().remove_pipeline("UI Pipeline");
  }

  void server::core_update() {
    pump_events();
  }

  void server::update_initializing() {}

  void server::update_running() {}

  void server::update_shutting_down() {}

  void server::validate_project_and_launch(const json::json& project_entry) {
    std::string name = project_entry.at("name").get<std::string>();

    json::json launch_info = project_entry.at("build");
    std::string type = launch_info.at("type").get<std::string>();

    enum class launch_type {
      OTHER_APPLICATION_EXE,
      UNKNOWN,
    };

    launch_type ltype = launch_type::UNKNOWN;
    if (type == "other-application") {
      ltype = launch_type::OTHER_APPLICATION_EXE;
    } else {
      CORE_LOG_ERROR("Unknown launch type '{}' for project '{}'", type, name);
      return;
    }

    switch (ltype) {
      case launch_type::OTHER_APPLICATION_EXE: {
        begin_other_application(project_entry);
      } break;

      case launch_type::UNKNOWN:
      default:
        CORE_LOG_ERROR("Unhandled launch type for project '{}'", name);
        break;
    }
  }

  std::string replace_all_substrings_with(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();  // Move past the replaced substring to avoid infinite loops if 'to' contains 'from'
    }
    return str;
  }

  void server::begin_other_application(const json::json& project_entry) {
    /// launch other application with command line args specifying the server's current command port, the project configuration
    ///   and the working directory
    /// set an event listener for 'session-initial-check-in' to register other-application with the server
    /// respond to the ping with server data

#if 0
  #define TESTING_COROS
#endif
    // post_coroutine(validate_and_build_other_application("OtherApp", working_dir, exe_name));
#ifdef TESTING_COROS
#else

    std::string name = project_entry.at("name").get<std::string>();
    filepath project_file = filepath(project_entry.at("project-file").get<std::string>());
    filepath working_dir = project_entry.at("working-directory").get<std::string>();
    json::json build_info = project_entry.at("build");

    opt<filepath> output_file = build_info.contains("output-file") ? filepath(build_info.at("output-file").get<std::string>()) : opt<filepath>{};
    filepath exe_name = build_info.contains("executable") ? filepath(build_info.at("executable").get<std::string>()) : filepath("OtherApp.exe");
    std::vector<std::string> args = build_info.contains("args") ? build_info.at("args").get<std::vector<std::string> >() : std::vector<std::string>{};

    static integer_t next_id = 1;
    integer_t id = next_id++;
    auto itr = app_list.pending_apps.insert(app_list.pending_apps.end(), application_list::other_application{ .id = id, .working_directory = working_dir, .executable = exe_name, .args = args });
    OTHER_ASSERT(itr != app_list.pending_apps.end(), "Failed to begin Other application : {}  [{}]", id, exe_name.string());

    CORE_LOG_DEBUG("Starting Other application : {}", id);
    itr->executable = replace_all_substrings_with(itr->executable->string(), "${configuration}", "Debug");

    CORE_LOG_DEBUG("Launching Other application executable '{}' @ [{}]:", itr->executable->string(), working_dir.string());
    for (const auto& arg : itr->args) {
      CORE_LOG_DEBUG("   - {}", arg);
    }

    /// \todo: build the project and validate it is correct first
    // project_description proj_desc = {
    //   .project_type = project_description::APPLICATION,
    //   .name = name,
    //   .working_directory = working_dir,
    //   .output_directory = output_file.has_value() ? output_file->parent_path() : working_dir / filepath("build"),
    //   .exe_name = output_file.has_value() ? *output_file : working_dir / filepath("build") / exe_name,
    //   .configurations = { "Debug", "Release" },
    //   .active_configuration = 0,
    //   .cmd_args = args,
    //   .version = "0.1.0",
    //   .description = "An Other application.",
    //   .author = "Author Name",
    //   .license = "MIT",
    // };
    // if (project_entry.contains("project-file")) {
    //   proj_desc.override_file_name = project_entry.at("project-file").get<std::string>();
    // }
    // build_tool bt;
    // bt.start_build(proj_desc);

    session_check_in_request(itr->id);

    itr->args.insert(itr->args.begin(), project_file.string());
    itr->args.append_range(std::vector<std::string>{ "--sid", std::to_string(itr->id) });
    itr->args.append_range(std::vector<std::string>{ "--port", std::to_string(net_context->main_binding_point.port) });
    launch_detached_process(*itr->working_directory, *itr->executable, itr->args);
#endif
  }

  // void server::on_timeout_request_session_information_network_thread(message_header header) {
  //   CORE_LOG_WARN("Network thread timed out waiting for session information response for session [{}]", header.id);
  // }

  void server::on_notification_session_closed(integer_t session_id) {
    auto app_itr = app_list.other_apps.find(session_id);
    if (app_itr != app_list.other_apps.end()) {
      CORE_LOG_INFO("Other application [{}] has disconnected", app_itr->second.get_name());
      app_itr->second.connected = false;
      /// don't remove from list, it might reconnect
      /// \todo set a timeout to remove it after a while if needed
    } else {
      /// ignore because there may be open sessions that aren't other applications
    }
  }

  task server::validate_and_build_other_application(const std::string& name, const filepath& folder, const filepath& env_config_path) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

    integer_t builder_obj_id = env->create_object("Builder");
    OTHER_ASSERT(builder_obj_id != -1, "Failed to create Builder object in scripting environment");

    env->attach_dotnet_object(builder_obj_id, "Other.BuildTool");
    // co_await task::awaiter{};

    CORE_LOG_DEBUG("Validating and building Other application '{}' in folder '{}'", name, folder.string());
    script_object* builder_obj = env->get_object(builder_obj_id);
    OTHER_ASSERT(builder_obj != nullptr, "Failed to retrieve Builder object from scripting environment");

    builder_obj->dotnet_object->invoke("ValidateAndBuildOtherApplication");  //, name, folder.string(), env_config_path.string());

    env->destroy_object(builder_obj_id);
    co_return;
  }

}  // namespace other