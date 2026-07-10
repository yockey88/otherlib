/**
 * \file tests/scripting/script_environment_tests.cpp
 **/
#include "script_environment_tests.hpp"

#include "gtest/gtest.h"

namespace other {

  TEST_F(script_environment_tests, call_static_dotnet_method) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    {
      integer_t obj_id = 0;
      EXPECT_NO_FATAL_FAILURE(obj_id = env->create_object("MyObject"));
      ASSERT_GE(obj_id, 0);

      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      EXPECT_EQ(script_obj->dotnet_object, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(obj_id, "TestObject"));
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      int res = -1;
      EXPECT_NO_FATAL_FAILURE(res = script_obj->dotnet_object->invoke<int>("StaticMethod", int{ 5 }));
      EXPECT_EQ(res, 25);

      EXPECT_NO_FATAL_FAILURE(env->destroy_object(obj_id));
    }

    {
      int res = -1;
      EXPECT_NO_FATAL_FAILURE(res = env->call_static_dotnet_method<int>("TestObject", "StaticMethod", int{ 6 }));
      EXPECT_EQ(res, 36);
    }
  }

}  // namespace other