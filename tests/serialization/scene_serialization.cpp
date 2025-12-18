/**
 * \file tests/serialization/scene_serialization.cpp
 **/
#include "scene/scene_serialization.hpp"

#include "script/script_object.hpp"

#include "object/object_serialization.hpp"
#include "object/scene_object.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serialization_data.hpp"

#include "serialization_tests.hpp"

namespace other {

  TEST_F(serialization_tests, write_object_to_bytes) {
    std::vector<uint8_t> bytes = {};
    {
      scene scene1("Test Scene");
      scene_object& obj = scene1.create_object("Test Object");
      transform& obj_transform = scene1.get_transform(&obj);
      obj_transform.local_position = { 1.0f, 2.0f, 3.0f };
      obj_transform.local_rotation_quat = { 0.1f, 0.2f, 0.3f, 1.0f };
      obj_transform.local_scale = { 2.0f, 2.0f, 2.0f };
      bytes = serialization::write_object_to_bytes(scene1, obj);
      EXPECT_GT(bytes.size(), 0);
    }

    {
      scene s1("Temp Scene");

      auto [parsed_obj, bytes_read] = serialization::parse_single_object(std::span<const uint8_t>{ bytes.data(), bytes.size() });
      ASSERT_EQ(bytes_read, bytes.size());

      std::vector objects{ parsed_obj };
      s1.add_objects(objects);

      scene_object& obj1 = s1.get_object(parsed_obj.object.id);
      EXPECT_EQ(obj1.name, "Test Object");
      EXPECT_EQ(obj1.id, 1u);

      transform& obj1_transform = s1.get_transform(&obj1);
      EXPECT_EQ(obj1_transform.local_position, glm::vec3(1.0f, 2.0f, 3.0f));
      EXPECT_EQ(obj1_transform.local_rotation_quat, glm::quat(0.1f, 0.2f, 0.3f, 1.0f));
      EXPECT_EQ(obj1_transform.local_scale, glm::vec3(2.0f, 2.0f, 2.0f));
    }
  }

  TEST_F(serialization_tests, write_scripted_object_to_bytes) {
    std::vector<uint8_t> bytes = {};

    {
      scene s1("Test Scene");
      scene_object& obj = s1.create_object("Test Object");
      transform& obj_transform = s1.get_transform(&obj);
      obj_transform.local_position = { 1.0f, 2.0f, 3.0f };
      obj_transform.local_rotation_quat = { 0.1f, 0.2f, 0.3f, 1.0f };
      obj_transform.local_scale = { 2.0f, 2.0f, 2.0f };

      script_component* comp = s1.get_component<script_component>(&obj);
      ASSERT_NE(comp, nullptr);

      ASSERT_NO_FATAL_FAILURE(subsystem<scripting_environment>::get()->attach_dotnet_object(comp->script_object_id, "TestObject"));
      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      script_obj->dotnet_object->set_field("field_value", 42);
      script_obj->dotnet_object->set_property("PropertyValue", 84);
      script_obj->dotnet_object->set_field("field_string", std::string("Hello There from Scripted Object"));
      script_obj->dotnet_object->set_property("PropertyString", std::string("Property Hello There from Scripted Object"));

      bytes = serialization::write_object_to_bytes(s1, obj);
      EXPECT_GT(bytes.size(), 0);
    }

    {
      scene s1("Temp Scene");

      auto [parsed_obj, bytes_read] = serialization::parse_single_object(std::span<const uint8_t>{ bytes.data(), bytes.size() });
      /// can't continue on if we didn't read all bytes, everything else will be invalid
      ASSERT_EQ(bytes_read, bytes.size());

      ASSERT_EQ(parsed_obj.object.id, 1u);
      ASSERT_EQ(parsed_obj.object.name, "Test Object");
      ASSERT_EQ(parsed_obj.parent_id, 0u);
      ASSERT_EQ(parsed_obj.children_ids.size(), 0u);
      ASSERT_EQ(parsed_obj.dotnet_obj.name, "TestObject");

      std::vector objects{ parsed_obj };
      s1.add_objects(objects);

      scene_object& obj1 = s1.get_object(parsed_obj.object.id);
      EXPECT_EQ(obj1.name, "Test Object");
      EXPECT_EQ(obj1.id, 1u);

      transform& obj1_transform = s1.get_transform(&obj1);
      EXPECT_EQ(obj1_transform.local_position, glm::vec3(1.0f, 2.0f, 3.0f));
      EXPECT_EQ(obj1_transform.local_rotation_quat, glm::quat(0.1f, 0.2f, 0.3f, 1.0f));
      EXPECT_EQ(obj1_transform.local_scale, glm::vec3(2.0f, 2.0f, 2.0f));

      script_component* comp1 = s1.get_component<script_component>(&obj1);
      ASSERT_NE(comp1, nullptr);

      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 42);
      EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 84);
      EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello There from Scripted Object");
      EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello There from Scripted Object");
    }
  }

  TEST_F(serialization_tests, full_scene_serialization) {
    std::vector<uint8_t> scene_bytes = {};
    size_t expected_size = 0;

    {
      scene scene1("Test Scene");

      scene_object& obj1 = scene1.create_object("Test Object 1");
      script_component* comp1 = scene1.get_component<script_component>(&obj1);
      ASSERT_NE(comp1, nullptr);
      ASSERT_NO_FATAL_FAILURE(subsystem<scripting_environment>::get()->attach_dotnet_object(comp1->script_object_id, "TestObject"));

      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      ASSERT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
      ASSERT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
      ASSERT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
      ASSERT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

      scene_object& obj2 = scene1.create_object("Test Object 2");
      script_component* comp2 = scene1.get_component<script_component>(&obj2);
      ASSERT_NE(comp2, nullptr);
      ASSERT_NO_FATAL_FAILURE(subsystem<scripting_environment>::get()->attach_dotnet_object(comp2->script_object_id, "TestObject"));

      script_object* script_obj2 = subsystem<scripting_environment>::get()->get_object(comp2->script_object_id);
      ASSERT_NE(script_obj2, nullptr);
      ASSERT_NE(script_obj2->dotnet_object, nullptr);
      script_obj2->dotnet_object->set_field("field_value", 777);
      script_obj2->dotnet_object->set_property("PropertyValue", 888);
      script_obj2->dotnet_object->set_field("field_string", std::string("Hello from Scene Serialization"));
      script_obj2->dotnet_object->set_property("PropertyString", std::string("Property Hello from Scene Serialization"));

      ASSERT_EQ(script_obj2->dotnet_object->get_field<int>("field_value"), 777);
      ASSERT_EQ(script_obj2->dotnet_object->get_property<int>("PropertyValue"), 888);
      ASSERT_EQ(script_obj2->dotnet_object->get_field<std::string>("field_string"), "Hello from Scene Serialization");
      ASSERT_EQ(script_obj2->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from Scene Serialization");

      /// leave script_obj1 untouched
      ASSERT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
      ASSERT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
      ASSERT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
      ASSERT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

      scene_object& child1 = scene1.create_object("Child Object 1", &obj1);
      scene_object& child2 = scene1.create_object("Child Object 2", &obj2);
      scene_object& child3 = scene1.create_object("Child Object 3", &obj2);

      [[maybe_unused]] scene_object& grandchild1 = scene1.create_object("Grandchild Object 1", &child1);
      [[maybe_unused]] scene_object& grandchild2 = scene1.create_object("Grandchild Object 2", &child1);
      [[maybe_unused]] scene_object& grandchild3 = scene1.create_object("Grandchild Object 3", &child2);
      [[maybe_unused]] scene_object& grandchild4 = scene1.create_object("Grandchild Object 4", &child3);

      scene_bytes = serialization::write_scene_to_bytes(scene1);
      EXPECT_GT(scene_bytes.size(), 0);
      expected_size = scene_bytes.size();
    }

    std::span<const uint8_t> buf{ scene_bytes.data(), scene_bytes.size() };
    auto [scene_desc, scene_name, bytes_read] = serialization::parse_single_scene(std::span<const uint8_t>{ scene_bytes.data(), scene_bytes.size() });
    ASSERT_EQ(bytes_read, serialization::kSceneNameOffset + std::string("Test Scene").length() + sizeof(uint16_t));

    std::span<const uint8_t> object_list_buf = std::span<const uint8_t>{ scene_bytes.data() + bytes_read, scene_bytes.size() - bytes_read };
    auto [objects, obj_bytes_read] = serialization::parse_object_list(object_list_buf, scene_desc.num_objects);
    ASSERT_EQ(bytes_read + obj_bytes_read, expected_size);

    {
      scene scene1;
      scene1.name = scene_name;
      scene1.id = scene_desc.scene_id;
      EXPECT_EQ(scene1.name, "Test Scene");
      EXPECT_EQ(scene1.id, FNV("Test Scene"));

      scene1.add_objects(objects);
      ASSERT_EQ(scene1.get_object_count(), scene_desc.num_objects);

      scene_object& obj1 = scene1.get_object(1);
      EXPECT_EQ(obj1.name, "Test Object 1");
      EXPECT_EQ(obj1.id, 1u);

      script_component* comp = scene1.get_component<script_component>(&obj1);
      ASSERT_NE(comp, nullptr);

      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
      EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
      EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
      EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

      scene_object& obj2 = scene1.get_object(2);
      EXPECT_EQ(obj2.name, "Test Object 2");
      EXPECT_EQ(obj2.id, 2u);

      script_component* comp2 = scene1.get_component<script_component>(&obj2);
      ASSERT_NE(comp2, nullptr);

      script_object* script_obj2 = subsystem<scripting_environment>::get()->get_object(comp2->script_object_id);
      ASSERT_NE(script_obj2, nullptr);
      ASSERT_NE(script_obj2->dotnet_object, nullptr);

      EXPECT_EQ(script_obj2->dotnet_object->get_field<int>("field_value"), 777);
      EXPECT_EQ(script_obj2->dotnet_object->get_property<int>("PropertyValue"), 888);
      EXPECT_EQ(script_obj2->dotnet_object->get_field<std::string>("field_string"), "Hello from Scene Serialization");
      EXPECT_EQ(script_obj2->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from Scene Serialization");

      scene_object& child1 = scene1.get_object(3);
      EXPECT_EQ(child1.name, "Child Object 1");
      EXPECT_EQ(child1.id, 3u);
      EXPECT_EQ(scene1.get_parent(&child1)->id, obj1.id);

      scene_object& child2 = scene1.get_object(4);
      EXPECT_EQ(child2.name, "Child Object 2");
      EXPECT_EQ(child2.id, 4u);
      EXPECT_EQ(scene1.get_parent(&child2)->id, obj2.id);

      scene_object& child3 = scene1.get_object(5);
      EXPECT_EQ(child3.name, "Child Object 3");
      EXPECT_EQ(child3.id, 5u);
      EXPECT_EQ(scene1.get_parent(&child3)->id, obj2.id);

      scene_object& grandchild1 = scene1.get_object(6);
      EXPECT_EQ(grandchild1.name, "Grandchild Object 1");
      EXPECT_EQ(grandchild1.id, 6u);
      EXPECT_EQ(scene1.get_parent(&grandchild1)->id, child1.id);
      scene_object* grandchild1_parent = scene1.get_parent(&grandchild1);
      EXPECT_EQ(scene1.get_parent(grandchild1_parent)->id, obj1.id);

      scene_object& grandchild2 = scene1.get_object(7);
      EXPECT_EQ(grandchild2.name, "Grandchild Object 2");
      EXPECT_EQ(grandchild2.id, 7u);
      EXPECT_EQ(scene1.get_parent(&grandchild2)->id, child1.id);
      scene_object* grandchild2_parent = scene1.get_parent(&grandchild2);
      EXPECT_EQ(scene1.get_parent(grandchild2_parent)->id, obj1.id);

      scene_object& grandchild3 = scene1.get_object(8);
      EXPECT_EQ(grandchild3.name, "Grandchild Object 3");
      EXPECT_EQ(grandchild3.id, 8u);
      EXPECT_EQ(scene1.get_parent(&grandchild3)->id, child2.id);
      scene_object* grandchild3_parent = scene1.get_parent(&grandchild3);
      EXPECT_EQ(scene1.get_parent(grandchild3_parent)->id, obj2.id);

      scene_object& grandchild4 = scene1.get_object(9);
      EXPECT_EQ(grandchild4.name, "Grandchild Object 4");
      EXPECT_EQ(grandchild4.id, 9u);
      EXPECT_EQ(scene1.get_parent(&grandchild4)->id, child3.id);
      scene_object* grandchild4_parent = scene1.get_parent(&grandchild4);
      EXPECT_EQ(scene1.get_parent(grandchild4_parent)->id, obj2.id);
    }
  }

  namespace {
    constexpr auto kFilePath = "tests/resources/scenes/serialization-tests.oscn";
    constexpr auto kFilePath2 = "tests/resources/scenes/serialization-tests-2.oscn";
  }  // namespace

  TEST_F(serialization_tests, write_scene_to_file) {
    GTEST_SKIP() << "Strange C# 'Fatal error 0xC0000005' failure, needs investigation";

    std::vector<uint8_t> scene_bytes = {};
    {
      scene scene1("File Test Scene");
      scene_object& obj1 = scene1.create_object("File Test Object 1");
      scene_object& obj2 = scene1.create_object("File Test Object 2", &obj1);

      script_component* comp1 = scene1.get_component<script_component>(&obj1);
      ASSERT_NE(comp1, nullptr);

      ASSERT_NO_FATAL_FAILURE(subsystem<scripting_environment>::get()->attach_dotnet_object(comp1->script_object_id, "TestObject"));
      script_object* script_obj1 = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
      ASSERT_NE(script_obj1, nullptr);
      ASSERT_NE(script_obj1->dotnet_object, nullptr);

      ASSERT_NO_FATAL_FAILURE(script_obj1->dotnet_object->set_field<int>("field_value", 123));
      ASSERT_NO_FATAL_FAILURE(script_obj1->dotnet_object->set_property<int>("PropertyValue", 456));
      ASSERT_NO_FATAL_FAILURE(script_obj1->dotnet_object->set_field<std::string>("field_string", "Hello from File Test"));
      ASSERT_NO_FATAL_FAILURE(script_obj1->dotnet_object->set_property<std::string>("PropertyString", "Property Hello from File Test"));

      script_obj1->dotnet_object->invoke("DisplayInfo");

      ASSERT_NO_FATAL_FAILURE(scene_bytes = serialization::write_scene_to_bytes(scene1));
    }

    std::ofstream outfile(kFilePath, std::ios::binary);
    OTHER_ASSERT(outfile.is_open(), "Failed to open file '{}' for writing", kFilePath);
    outfile.write(reinterpret_cast<const char*>(scene_bytes.data()), scene_bytes.size());
  }

  TEST_F(serialization_tests, read_scene_from_file) {
    std::ifstream infile(kFilePath, std::ios::binary);
    OTHER_ASSERT(infile.is_open(), "Failed to open file '{}' for reading", kFilePath);

    infile.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(infile.tellg());
    infile.seekg(0, std::ios::beg);
    OTHER_ASSERT(file_size > 0, "File '{}' is empty", kFilePath);

    std::vector<uint8_t> buffer(file_size);
    infile.read(reinterpret_cast<char*>(buffer.data()), file_size);

    std::span<const uint8_t> buf{ buffer.data(), buffer.size() };
    auto [scene1, bytes_read] = serialization::parse_scene(buf);
    ASSERT_EQ(scene1.name, "File Test Scene");
    ASSERT_EQ(bytes_read, buffer.size());

    std::string object_tree_string = "";
    ASSERT_NO_FATAL_FAILURE(object_tree_string = serialization::get_entity_tree_string(buf));
    std::println("Scene Object Hierarchy:\n{}", object_tree_string);

    scene_object& obj1 = scene1.get_object(1);
    EXPECT_EQ(obj1.name, "File Test Object 1");
    EXPECT_EQ(obj1.id, 1u);

    scene_object& obj2 = scene1.get_object(2);
    EXPECT_EQ(obj2.name, "File Test Object 2");
    EXPECT_EQ(obj2.id, 2u);

    script_component* comp1 = scene1.get_component<script_component>(&obj1);
    ASSERT_NE(comp1, nullptr);

    script_object* script_obj1 = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
    ASSERT_NE(script_obj1, nullptr);
    ASSERT_NE(script_obj1->dotnet_object, nullptr);

    EXPECT_EQ(script_obj1->dotnet_object->get_field<int>("field_value"), 123);
    EXPECT_EQ(script_obj1->dotnet_object->get_property<int>("PropertyValue"), 456);
    EXPECT_EQ(script_obj1->dotnet_object->get_field<std::string>("field_string"), "Hello from File Test");
    EXPECT_EQ(script_obj1->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from File Test");
  }

  /**
   * \note noticed the properties loaded with the wrong type if you did not set/get them first so the serialization
   *         would become corrupt
   **/
  TEST_F(serialization_tests, write_scene_to_file_edge_case) {
    GTEST_SKIP() << "Strange C# 'Fatal error 0xC0000005' failure, needs investigation";

    std::vector<uint8_t> scene_bytes = {};
    {
      scene scene1("File Test Scene");
      scene_object& obj1 = scene1.create_object("File Test Object 1");
      scene_object& obj2 = scene1.create_object("File Test Object 2", &obj1);

      script_component* comp1 = scene1.get_component<script_component>(&obj1);
      ASSERT_NE(comp1, nullptr);

      ASSERT_NO_FATAL_FAILURE(subsystem<scripting_environment>::get()->attach_dotnet_object(comp1->script_object_id, "TestObject"));
      script_object* script_obj1 = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
      ASSERT_NE(script_obj1, nullptr);
      ASSERT_NE(script_obj1->dotnet_object, nullptr);

      script_obj1->dotnet_object->invoke("DisplayInfo");

      ASSERT_NO_FATAL_FAILURE(scene_bytes = serialization::write_scene_to_bytes(scene1));
    }

    std::ofstream outfile(kFilePath2, std::ios::binary);
    OTHER_ASSERT(outfile.is_open(), "Failed to open file '{}' for writing", kFilePath2);
    outfile.write(reinterpret_cast<const char*>(scene_bytes.data()), scene_bytes.size());
  }

  TEST_F(serialization_tests, read_scene_from_file_edge_case) {
    GTEST_SKIP() << "Strange C# 'Fatal error 0xC0000005' failure, needs investigation";

    std::ifstream infile(kFilePath2, std::ios::binary);
    OTHER_ASSERT(infile.is_open(), "Failed to open file '{}' for reading", kFilePath2);

    infile.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(infile.tellg());
    infile.seekg(0, std::ios::beg);
    OTHER_ASSERT(file_size > 0, "File '{}' is empty", kFilePath2);

    std::vector<uint8_t> buffer(file_size);
    infile.read(reinterpret_cast<char*>(buffer.data()), file_size);

    std::span<const uint8_t> buf{ buffer.data(), buffer.size() };
    auto [scene1, bytes_read] = serialization::parse_scene(buf);
    ASSERT_EQ(scene1.name, "File Test Scene");
    ASSERT_EQ(bytes_read, buffer.size());

    std::string object_tree_string = "";
    ASSERT_NO_FATAL_FAILURE(object_tree_string = serialization::get_entity_tree_string(buf));
    std::println("Scene Object Hierarchy:\n{}", object_tree_string);

    scene_object& obj1 = scene1.get_object(1);
    EXPECT_EQ(obj1.name, "File Test Object 1");
    EXPECT_EQ(obj1.id, 1u);

    scene_object& obj2 = scene1.get_object(2);
    EXPECT_EQ(obj2.name, "File Test Object 2");
    EXPECT_EQ(obj2.id, 2u);

    script_component* comp1 = scene1.get_component<script_component>(&obj1);
    ASSERT_NE(comp1, nullptr);

    script_object* script_obj1 = subsystem<scripting_environment>::get()->get_object(comp1->script_object_id);
    ASSERT_NE(script_obj1, nullptr);
    ASSERT_NE(script_obj1->dotnet_object, nullptr);

    ASSERT_NO_FATAL_FAILURE(script_obj1->dotnet_object->invoke("DisplayInfo"));
    EXPECT_EQ(script_obj1->dotnet_object->get_field<int>("field_value"), 10);
    EXPECT_EQ(script_obj1->dotnet_object->get_property<int>("PropertyValue"), 20);
    EXPECT_EQ(script_obj1->dotnet_object->get_field<std::string>("field_string"), "Hello World");
    EXPECT_EQ(script_obj1->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");
  }

}  // namespace other