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

namespace other {

#pragma pack(push, 1)
  struct project_description {
    uint16_t magic_header = 0;
    uint8_t version[3] = {};
    uint16_t num_scenes = 0;
  };

  constexpr static uint16_t kProjectFileMagicHeader = 0x594F;  // 'OY'

  constexpr static natural_t kSceneListOffset = sizeof(project_description);
  constexpr static natural_t kSceneNameOffset = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint16_t);

  struct parsed_scene {
    uint8_t marker_bytes = 0xFF;
    uint8_t scene_data_type = 0;  // 0 = external, 1 = inline
    uint8_t scene_index = 0;
    uint64_t scene_id = 0;
    uint16_t scene_name_length = 0;
    /// name
    uint16_t num_objects = 0;
    /// objects

    enum scene_data_type : uint8_t {
      EXTERNAL = 0,
      INLINE = 1
    };
  };
#pragma pack(pop)

  namespace detail {

    uint16_t create_u16(uint8_t hi, uint8_t lo) {
      return (hi << 8) | lo;
    }

    project_description parse_project_description(const std::span<const uint8_t> buffer) {
      project_description proj = *reinterpret_cast<const project_description*>(buffer.data());
      return proj;
    }

    std::tuple<std::string, parsed_scene, natural_t> parse_scene_description(const std::span<const uint8_t> buffer) {
      OTHER_ASSERT(sizeof(parsed_scene) < buffer.size(), "Buffer too small to contain scene description");
      uint8_t* scene_buffer_ptr = const_cast<uint8_t*>(buffer.data());
      parsed_scene* scene_ptr = reinterpret_cast<parsed_scene*>(scene_buffer_ptr);

      OTHER_ASSERT(kSceneNameOffset + scene_ptr->scene_name_length < buffer.size(), "Buffer too small to contain scene name");
      std::string scene_name = std::string{ reinterpret_cast<const char*>(buffer.data() + kSceneNameOffset), scene_ptr->scene_name_length };

      const uint8_t* object_count_ptr = buffer.data() + kSceneNameOffset + scene_ptr->scene_name_length;
      scene_ptr->num_objects = *reinterpret_cast<const uint16_t*>(object_count_ptr);

      std::println("Number of objects in scene '{}': {}", scene_name, scene_ptr->num_objects);
      return { scene_name, *scene_ptr, kSceneNameOffset + scene_ptr->scene_name_length + sizeof(uint16_t) };
    }

  }  // namespace detail

#if 0
  #define WRITING
#else
  #define READING
#endif

  void runtime::on_initialize() {
    other_assembly = load_dotnet_module("build/other-csharp/Debug/OtherCs.dll");
    testing_assembly = load_dotnet_module("build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll");

#if 1
    scene scene1 = {};
    {
      scene1.name = "Scene 1";
      scene1.id = FNV(scene1.name);
      std::println("Created scene '{}' with ID {:#018x}", scene1.name, scene1.id);

      scene_object& obj1 = scene1.create_object("Test Object 1");
      script_component* script_comp = scene1.get_component<script_component>(&obj1);
      std::vector<uint8_t> bytes = {};

      auto* env = subsystem<scripting_environment>::get();
      {
        env->attach_dotnet_object(script_comp->script_object_id, "TestObject");

        script_object* script_obj = env->get_object(script_comp->script_object_id);
        script_obj->dotnet_object->set_field("field_value", 15);
        script_obj->dotnet_object->invoke("DisplayInfo");

        bytes.append_range(script_obj->dotnet_object->serialize_to_bytes());
        env->detach_dotnet_object(script_comp->script_object_id);
      }

      {
        env->attach_dotnet_object(script_comp->script_object_id, "TestObject");

        script_object* script_obj = env->get_object(script_comp->script_object_id);
        script_obj->dotnet_object->load_from_bytes(bytes);
        script_obj->dotnet_object->invoke("DisplayInfo");

        // env->detach_dotnet_object(script_comp->script_object_id);
      }
    }

#else
  #ifdef WRITING
    // scripting_environment* env = subsystem<scripting_environment>::get();
    // OTHER_ASSERT(env != nullptr, "Scripting environment subsystem not found");

    // script_component* comp = scene1.get_component<script_component>(&obj1);
    // OTHER_ASSERT(comp != nullptr, "Script component not found on object");
    // env->attach_dotnet_object(comp->script_object_id, "TestObject");

    // std::vector<uint8_t> data = serialization::write_attached_scripts_to_bytes(comp);
    // {
    //   std::stringstream ss;
    //   for (uint32_t i = 0; i < data.size(); ++i) {
    //     ss << std::format("{:02X} ", data[i]);
    //     if ((i + 1) % 16 == 0 && i > 0) {
    //       ss << "\n";
    //     }
    //   }
    //   std::println("Serialized attached scripts:\n{}", ss.str());
    // }
    // scene_object& obj2 = scene1.create_object("Test Object 2");

    // scene_object& child1 = scene1.create_object("Child Object 1", &obj1);
    // scene_object& child2 = scene1.create_object("Child Object 2", &obj2);
    // scene_object& child3 = scene1.create_object("Child Object 3", &obj2);

    // scene_object& grandchild1 = scene1.create_object("Grandchild Object 1", &child1);

    {
      std::ofstream file("resources/dev-project0.other", std::ios::binary | std::ios::trunc);
      OTHER_ASSERT(file.is_open(), "Failed to open project file for writing");

      std::vector<uint8_t> scene_bytes = serialization::write_scene_to_bytes(scene1);
      project_description proj_desc = {};
      proj_desc.magic_header = kProjectFileMagicHeader;
      proj_desc.version[0] = 0;
      proj_desc.version[1] = 0;
      proj_desc.version[2] = 1;
      proj_desc.num_scenes = 1;
      const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&proj_desc);
      file.write(reinterpret_cast<const char*>(header_bytes), sizeof(project_description));
      file.write(reinterpret_cast<const char*>(scene_bytes.data()), scene_bytes.size());
      file.close();
    }
  #endif

  #ifdef READING
    std::vector<uint8_t> buffer = {};
    {
      std::ifstream file("resources/dev-project0.other", std::ios::binary);
      OTHER_ASSERT(file.is_open(), "Failed to open project file");

      size_t num_bytes = 0;
      file.seekg(0, std::ios::end);
      num_bytes = static_cast<size_t>(file.tellg());
      file.seekg(0, std::ios::beg);
      OTHER_ASSERT(num_bytes > 0, "Project file is empty");

      buffer.resize(num_bytes);
      file.read(reinterpret_cast<char*>(buffer.data()), num_bytes);
      std::println("Read {} bytes from project file", buffer.size());
    }

    std::span<const uint8_t> buf{ buffer.data(), buffer.size() };
    project_description proj = detail::parse_project_description(buf);

    std::span<const uint8_t> scene_list_buf = buf.subspan(kSceneListOffset);
    // std::println("{}", serialization::get_entity_tree_string(scene_list_buf));
    auto [scenes, bytes_read] = serialization::parse_scene_list(scene_list_buf, proj.num_scenes);

    auto& scene1 = scenes.at(0);
    std::println("Loaded scene '{}' with ID {:#018x}", scene1.name, scene1.id);

    scene_object& obj1 = scene1.get_object(1);
    {
      scripting_environment* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem not found");

      script_component* comp = scene1.get_component<script_component>(&obj1);
      OTHER_ASSERT(comp != nullptr, "Script component not found on object");

      script_object* script_obj = env->get_object(comp->script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment", comp->script_object_id);

      if (script_obj->dotnet_object == nullptr) {
        std::println("Failed to load .NET object for script object ID {}", comp->script_object_id);
      }
    }
  #endif
#endif

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