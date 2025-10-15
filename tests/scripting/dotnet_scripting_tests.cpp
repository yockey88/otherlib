/**
 * \file scripting/dotnet_scripting_tests.cpp
 **/
#include "scripting/dotnet_scripting_tests.hpp"

namespace other {

  TEST_F(dotnet_scripting_tests, attach_and_verify_dotnet_object) {
    integer_t object_id = -1;
    ASSERT_NO_FATAL_FAILURE(object_id = subsystem<scripting_environment>::get()->create_object("Test Object 1"));
    ASSERT_NE(object_id, -1);

    subsystem<scripting_environment>::get()->attach_dotnet_object(object_id, "TestObject");
    script_object* script_obj = subsystem<scripting_environment>::get()->get_object(object_id);
    ASSERT_NE(script_obj, nullptr);

    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_value", 12345));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyValue", 67890));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_string", std::string("Hello from DotNet Object")));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyString", std::string("Property Hello from DotNet Object")));

    EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 12345);
    EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 67890);
    EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello from DotNet Object");
    EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from DotNet Object");

    subsystem<scripting_environment>::get()->detach_dotnet_object(object_id);
  }

  TEST_F(dotnet_scripting_tests, reattach_dotnet_object) {
    integer_t object_id = -1;
    ASSERT_NO_FATAL_FAILURE(object_id = subsystem<scripting_environment>::get()->create_object("Test Object 1"));
    ASSERT_NE(object_id, -1);

    subsystem<scripting_environment>::get()->attach_dotnet_object(object_id, "TestObject");
    script_object* script_obj = subsystem<scripting_environment>::get()->get_object(object_id);
    ASSERT_NE(script_obj, nullptr);

    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_value", 12345));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyValue", 67890));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_string", std::string("Hello from DotNet Object")));
    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_property("PropertyString", std::string("Property Hello from DotNet Object")));

    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->invoke("DisplayInfo"));

    EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 12345);
    EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 67890);
    EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello from DotNet Object");
    EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Property Hello from DotNet Object");

    subsystem<scripting_environment>::get()->detach_dotnet_object(object_id);

    subsystem<scripting_environment>::get()->attach_dotnet_object(object_id, "TestObject");
    script_obj = subsystem<scripting_environment>::get()->get_object(object_id);
    ASSERT_NE(script_obj, nullptr);

    EXPECT_EQ(script_obj->dotnet_object->get_field<int>("field_value"), 10);
    EXPECT_EQ(script_obj->dotnet_object->get_property<int>("PropertyValue"), 20);
    EXPECT_EQ(script_obj->dotnet_object->get_field<std::string>("field_string"), "Hello World");
    EXPECT_EQ(script_obj->dotnet_object->get_property<std::string>("PropertyString"), "Hello Property");

    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->invoke("DisplayInfo"));

    subsystem<scripting_environment>::get()->detach_dotnet_object(object_id);
  }

  TEST_F(dotnet_scripting_tests, invoke_method_on_dotnet_object) {
    integer_t object_id = -1;
    ASSERT_NO_FATAL_FAILURE(object_id = subsystem<scripting_environment>::get()->create_object("Test Object 1"));
    ASSERT_NE(object_id, -1);

    subsystem<scripting_environment>::get()->attach_dotnet_object(object_id, "TestObject");
    script_object* script_obj = subsystem<scripting_environment>::get()->get_object(object_id);
    ASSERT_NE(script_obj, nullptr);

    ASSERT_NO_FATAL_FAILURE(script_obj->dotnet_object->invoke("DisplayInfo"));

    subsystem<scripting_environment>::get()->detach_dotnet_object(object_id);
  }

}  // namespace other