/**
 * \file runtime-dev/runtime.cpp
 **/
#include "runtime.hpp"

#include <cstddef>
#include <cstdint>

#include "serialization/reflection.hpp"

#include "script/scripting_environment.hpp"

#include "object/object_serialization.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene_serialization.hpp"
#include "scene/scene_serialization_data.hpp"

#include "project/project_serialization.hpp"

namespace other {

  /// goal 1: bind this and call from C#
  void hello_cs() {
    std::println("Hello There!");
  }

  /// goal 2: bind and call from C#
  struct my_foo {
    void hello_cs() {
      std::println("Hello There! {:p}", (void*)this);
    }
  };

  namespace detail {

    // project_description parse_project_description(const std::span<const uint8_t> buffer) {
    //   project_description proj = *reinterpret_cast<const project_description*>(buffer.data());
    //   return proj;
    // }

    std::vector<uint8_t> read_file(const std::string_view filepath) {
      std::ifstream file(std::string{ filepath }, std::ios::binary);
      OTHER_ASSERT(file.is_open(), "Failed to open project file");

      std::vector<uint8_t> buffer = {};

      size_t num_bytes = 0;
      file.seekg(0, std::ios::end);
      num_bytes = static_cast<size_t>(file.tellg());
      file.seekg(0, std::ios::beg);
      OTHER_ASSERT(num_bytes > 0, "Project file is empty");
      std::println("Project file size: {} bytes", num_bytes);

      buffer.resize(num_bytes);
      file.read(reinterpret_cast<char*>(buffer.data()), num_bytes);

      return buffer;
    }

  }  // namespace detail

  void runtime::on_initialize(const command_line& cmd) {
    CORE_LOG_DEBUG("Runtime...");

    // scene_object& obj = active_scene.create_object("Runtime-Test-Object");
    // script_component* comp = active_scene.get_component<script_component>(&obj);
    // OTHER_ASSERT(comp != nullptr, "Failed to create script component on test object");

    // auto* env = subsystem<scripting_environment>::get();
    // OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

    // env->attach_dotnet_object(comp->script_object_id, "TestObject");

    // script_object* script_obj = env->get_object(comp->script_object_id);
    // OTHER_ASSERT(script_obj != nullptr, "Failed to retrieve script object from scripting environment");

    // script_obj->dotnet_object->invoke("DisplayInfo");

    integer_t session_id = cmd.session_id.value_or(-1);
    uint16_t port = cmd.port.value_or(49222);

    net_thread = make_scope<network_thread>(net_thread_message_bus);
    net_thread->launch();
    net_thread_message_bus.register_thread();

    if (session_id == -1) {
      CORE_LOG_WARN("No session ID provided to runtime");
    } else {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SESSION_CHECK_IN,
      };

      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
      const uint8_t* port_bytes = reinterpret_cast<const uint8_t*>(&port);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      msg.data.append_range(std::span(port_bytes, sizeof(uint16_t)));
      net_thread_message_bus.send_message(std::move(msg));
    }

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

    builder_obj_id = env->create_object("Builder");
    OTHER_ASSERT(builder_obj_id != -1, "Failed to create Builder object in scripting environment");

    env->attach_dotnet_object(builder_obj_id, "Other.BuildTool");

    // std::vector<uint8_t> buffer = detail::read_file("resources/dev-project1.other");
    // std::println("Read {} bytes from project file", buffer.size());

    // size_t cur = 0;
    // std::span<const uint8_t> buf{ buffer.data(), buffer.size() };
    // {
    //   auto proj_description = detail::parse_project_description(buf);
    //   cur = kSceneListOffset;
    //   CORE_LOG_DEBUG("Project version : [{}.{}.{}]", proj_description.version[0], proj_description.version[1], proj_description.version[2]);
    //   CORE_LOG_DEBUG("     - {} scenes", proj_description.num_scenes);

    //   auto [scenes, scene_list_bytes_read] = serialization::parse_scene_list(buf.subspan(cur), proj_description.num_scenes);
    //   cur += scene_list_bytes_read;

    //   // CORE_LOG_DEBUG("Parsed project description: name='{}', num_scenes={}", std::string{ proj_description.project_name }, proj_description.num_scenes);
    //   project_scene_graph = make_scope<scene_graph>(scenes);
    // }

    // initialize_subsystems();
    // load_project_configuration();
    // load_scenes_and_play();
    running = true;
  }

  void runtime::run() {
    do {
      pump_events();
      // update();
      // draw();
    } while (running);
  }

  void runtime::on_shutdown() {
    CORE_LOG_DEBUG("Shutting down runtime...");
    auto* env = subsystem<scripting_environment>::get();
    if (env && builder_obj_id != -1) {
      env->destroy_object(builder_obj_id);
      builder_obj_id = -1;
    }

    running = false;
    project_scene_graph = nullptr;

    net_thread->shutdown();
    net_thread = nullptr;
  }

  void runtime::catch_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down runtime...", signal);
      running = false;
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signal);
    }
  }

  void runtime::update() {
  }

  void runtime::draw() {
  }

  void runtime::on_event(SDL_Event* event) {
    OTHER_ASSERT(event != nullptr, "Event is null");
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: running = false; break;
      default: break;
    }
  }

}  // namespace other