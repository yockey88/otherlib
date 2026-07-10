/**
 * \file tests/serialization/script_serialization.cpp
 **/
#include <cstdint>

#include "serialization/serialization.hpp"

#include "object/object_serialization.hpp"
#include "scene/scene.hpp"

#include "gtest/gtest.h"
#include "serialization_tests.hpp"

namespace other {

  TEST_F(serialization_tests, write_dotnet_object_to_bytes) {
    ostd::vector<uint8_t> bytes = {};

    scene s1("Test Scene");
    scene_object& obj = s1.create_object("Test Object");
    script_component* comp = s1.get_component<script_component>(&obj);
    ASSERT_NE(comp, nullptr);

    {
      subsystem<scripting_environment>::get()->attach_dotnet_object(comp->script_object_id, "TestObject");
      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      script_obj->dotnet_object->set_field("field_value", 12345);
      script_obj->dotnet_object->set_property("PropertyValue", 67890);
      script_obj->dotnet_object->set_field("field_string", std::string("Hello from DotNet Object"));
      script_obj->dotnet_object->set_property("PropertyString", std::string("Property Hello from DotNet Object"));

      ASSERT_NO_FATAL_FAILURE(bytes = script_obj->dotnet_object->serialize_to_bytes());
      EXPECT_GT(bytes.size(), 0);

      subsystem<scripting_environment>::get()->detach_dotnet_object(comp->script_object_id);
    }

    {
      subsystem<scripting_environment>::get()->attach_dotnet_object(comp->script_object_id, "TestObject");
      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
      EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
      EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
      EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

      script_obj->dotnet_object->load_from_bytes(bytes);

      EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 12345);
      EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 67890);
      EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello from DotNet Object");
      EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from DotNet Object");
    }
  }

  TEST_F(serialization_tests, write_scene_object_with_dotnet_to_bytes) {
    std::vector<uint8_t> bytes = {};
    size_t expected_data_length = 0;

    scene s1("Test Scene");
    scene_object& obj = s1.create_object("Test Object");
    script_component* comp = s1.get_component<script_component>(&obj);
    ASSERT_NE(comp, nullptr);

    {
      subsystem<scripting_environment>::get()->attach_dotnet_object(comp->script_object_id, "TestObject");
      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_value", 12345));
      ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyValue", 67890));
      ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_string", std::string("Hello from DotNet Object")));
      ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyString", std::string("Property Hello from DotNet Object")));

      ASSERT_NO_FATAL_FAILURE({
        ostd::vector<uint8_t> script_bytes = script_obj->dotnet_object->serialize_to_bytes();
        expected_data_length = script_bytes.size();
      });

      ASSERT_NO_FATAL_FAILURE(bytes = serialization::write_attached_scripts_to_bytes(comp));
      EXPECT_GT(bytes.size(), 0);

      subsystem<scripting_environment>::get()->detach_dotnet_object(comp->script_object_id);
    }

    {
      // size_t cursor = 0;
      // std::span<const uint8_t> buffer{ bytes.data(), bytes.size() };

      // uint16_t type_name_len = 0;
      // ASSERT_NO_FATAL_FAILURE(type_name_len = serialization::read_value<uint16_t>(buffer, cursor));
      // ASSERT_EQ(cursor, sizeof(uint16_t));

      // std::string type_name = "";
      // ASSERT_NO_FATAL_FAILURE(type_name = serialization::read_string_value(buffer, type_name_len, cursor));
      // ASSERT_EQ(type_name, "TestObject");
      // ASSERT_EQ(cursor, type_name.length() + sizeof(uint16_t));

      // subsystem<scripting_environment>::get()->attach_dotnet_object(comp->script_object_id, type_name);
      // script_object* script_obj = subsystem<scripting_environment>::get()->get_object(comp->script_object_id);
      // ASSERT_NE(script_obj, nullptr);
      // ASSERT_NE(script_obj->dotnet_object, nullptr);

      // EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
      // EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
      // EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
      // EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

      // uint16_t data_len = 0;
      // ASSERT_NO_FATAL_FAILURE(data_len = serialization::read_value<uint16_t>(buffer, cursor));
      // ASSERT_EQ(cursor, type_name.length() + sizeof(uint16_t) * 2);
      // ASSERT_EQ(data_len, expected_data_length);

      // script_obj->dotnet_object->load_from_bytes(buffer.subspan(cursor));

      // EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 12345);
      // EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 67890);
      // EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello from DotNet Object");
      // EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from DotNet Object");
    }
  }

}  // namespace other