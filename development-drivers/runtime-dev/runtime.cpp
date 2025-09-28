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

  namespace detail {

    project_description parse_project_description(const std::span<const uint8_t> buffer) {
      project_description proj = *reinterpret_cast<const project_description*>(buffer.data());
      return proj;
    }

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

  void runtime::on_initialize() {
    other_assembly = load_dotnet_module("build/other-csharp/Debug/OtherCs.dll");
    testing_assembly = load_dotnet_module("build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll");

    std::vector<uint8_t> buffer = detail::read_file("resources/scenes/test-scene.oscn");
    std::println("Read {} bytes from project file", buffer.size());
    size_t cur = 0;
    std::span<const uint8_t> buf{ buffer.data(), buffer.size() };
    auto [scene1, bytes_read] = serialization::parse_scene(buf);
    cur += bytes_read;

    std::println("{}", serialization::get_entity_tree_string(buf));

    scene_object& obj1 = scene1.get_object(1);
    script_component* comp = scene1.get_component<script_component>(&obj1);

    scripting_environment* env = subsystem<scripting_environment>::get();
    {
      script_object* script_obj = env->get_object(comp->script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment", comp->script_object_id);
      OTHER_ASSERT(script_obj->dotnet_object != nullptr, "Dotnet object is null for script object ID {}", comp->script_object_id);

      /// should have the values we set before
      script_obj->dotnet_object->invoke("DisplayInfo");
    }

    // initialize_subsystems();
    // load_project_configuration();
    // load_scenes_and_play();
    running = true;
  }

  void runtime::run() {
    // do {
    //   pump_events();
    //   update();
    //   draw();
    // } while (running);
  }

  void runtime::on_shutdown() {
  }

  // void runtime::initialize_subsystems() {
  // }

  // void runtime::load_project_configuration() {
  // }

  // void runtime::load_scenes_and_play() {
  // }

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