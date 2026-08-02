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

  /// the unload/reload halves of a script assembly refresh: invalidation must unhook the
  ///  behavior instance from its parent's C# behavior list (not just the native records) and
  ///  reattach must rebuild it, otherwise stale instances keep running after a hot reload
  TEST_F(script_environment_tests, behavior_invalidate_and_reattach) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    integer_t parent_id = -1;
    EXPECT_NO_FATAL_FAILURE(parent_id = env->create_object("HotReloadParent"));
    ASSERT_GE(parent_id, 0);
    EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(parent_id, "TestParentObject"));

    script_object* parent = env->get_object(parent_id);
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(parent->dotnet_object, nullptr);

    EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_behavior(parent_id, "TestBehavior"));
    ASSERT_EQ(parent->behavior_handles.size(), 1u);
    EXPECT_GE(parent->behavior_handles[0].script_object_id, 0);
    EXPECT_EQ(parent->dotnet_object->invoke<int>("CountBehaviors"), 1);

    dotnet_type* behavior_type = env->get_dotnet_host().get_type_cache()->get_type("TestBehavior");
    ASSERT_NE(behavior_type, nullptr);

    /// unload half (mirrors assembly_context::unload_assembly's per-type sequence)
    EXPECT_NO_FATAL_FAILURE(env->invalidate_dotnet_script_objects_of_type(behavior_type->dotnet_id));
    EXPECT_NO_FATAL_FAILURE(env->get_dotnet_host().purge_dotnet_type(behavior_type->dotnet_id));

    ASSERT_EQ(parent->behavior_handles.size(), 1u);
    EXPECT_EQ(parent->behavior_handles[0].script_object_id, -1);
    EXPECT_EQ(parent->dotnet_object->invoke<int>("CountBehaviors"), 0);

    /// reload half
    EXPECT_NO_FATAL_FAILURE(env->reattach_invalidated_dotnet_behaviors());
    ASSERT_EQ(parent->behavior_handles.size(), 1u);
    EXPECT_GE(parent->behavior_handles[0].script_object_id, 0);
    EXPECT_EQ(parent->dotnet_object->invoke<int>("CountBehaviors"), 1);

    EXPECT_NO_FATAL_FAILURE(env->detach_all_dotnet_behaviors(parent_id));
    EXPECT_EQ(parent->dotnet_object->invoke<int>("CountBehaviors"), 0);
    EXPECT_NO_FATAL_FAILURE(env->destroy_object(parent_id));
  }

}  // namespace other