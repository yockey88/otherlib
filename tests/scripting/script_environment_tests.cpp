/**
 * \file tests/scripting/script_environment_tests.cpp
 **/
#include "script_environment_tests.hpp"

#include <cstring>

#include "dotnet/dotnet_object.hpp"

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

  /// the managed-reflection pipeline behind the behavior inspector: GetFields/GetProperties
  ///  must enumerate public members (the BindingFlags regression returned none), the
  ///  snapshot walks each behavior's own dotnet type, and field edits round-trip by name
  TEST_F(script_environment_tests, behavior_snapshot_lists_fields_and_roundtrips) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    integer_t parent_id = -1;
    EXPECT_NO_FATAL_FAILURE(parent_id = env->create_object("SnapshotFieldsParent"));
    ASSERT_GE(parent_id, 0);
    EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(parent_id, "TestParentObject"));

    script_object* parent = env->get_object(parent_id);
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(parent->dotnet_object, nullptr);
    EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_behavior(parent_id, "TestBehavior"));

    behavior_snapshot snapshot = parent->get_behavior_snapshot();
    ASSERT_TRUE(snapshot.valid);
    ASSERT_EQ(snapshot.behaviors.size(), 1u);

    const behavior_descriptor& behavior = snapshot.behaviors[0];
    EXPECT_EQ(behavior.display_name, "TestBehavior");
    ASSERT_GE(behavior.script_object_id, 0);

    auto find_field = [&](std::string_view name) -> const behavior_field_descriptor* {
      for (const auto& f : behavior.fields) {
        if (f.field_name == name) {
          return &f;
        }
      }
      return nullptr;
    };

    const behavior_field_descriptor* speed = find_field("speed");
    ASSERT_NE(speed, nullptr);
    EXPECT_EQ(speed->type, value_type::FLOAT);
    const behavior_field_descriptor* counter = find_field("counter");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->type, value_type::INT32);
    const behavior_field_descriptor* active = find_field("active");
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->type, value_type::OEBOOL);
    const behavior_field_descriptor* label = find_field("label");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->type, value_type::STRING);

    script_object* behavior_obj = env->get_object(behavior.script_object_id);
    ASSERT_NE(behavior_obj, nullptr);
    ASSERT_NE(behavior_obj->dotnet_object, nullptr);

    float speed_val = 0.f;
    EXPECT_EQ(behavior_obj->dotnet_object->read_field_value("speed", &speed_val, sizeof(speed_val)), (int32_t)sizeof(float));
    EXPECT_FLOAT_EQ(speed_val, 2.5f);

    const float new_speed = 11.25f;
    EXPECT_TRUE(behavior_obj->dotnet_object->write_field_value("speed", &new_speed, sizeof(new_speed)));
    speed_val = 0.f;
    behavior_obj->dotnet_object->read_field_value("speed", &speed_val, sizeof(speed_val));
    EXPECT_FLOAT_EQ(speed_val, 11.25f);

    char text[64] = {};
    int32_t text_len = behavior_obj->dotnet_object->read_field_value("label", text, sizeof(text));
    EXPECT_EQ(std::string(text), "steady");
    EXPECT_EQ(text_len, (int32_t)std::string("steady").size() + 1);

    /// shrink then grow, the storage block reuse path both ways
    const char* short_label = "ok";
    EXPECT_TRUE(behavior_obj->dotnet_object->write_field_value("label", short_label, (int32_t)std::strlen(short_label) + 1));
    std::memset(text, 0, sizeof(text));
    behavior_obj->dotnet_object->read_field_value("label", text, sizeof(text));
    EXPECT_EQ(std::string(text), "ok");

    const char* long_label = "a longer label than before";
    EXPECT_TRUE(behavior_obj->dotnet_object->write_field_value("label", long_label, (int32_t)std::strlen(long_label) + 1));
    std::memset(text, 0, sizeof(text));
    behavior_obj->dotnet_object->read_field_value("label", text, sizeof(text));
    EXPECT_EQ(std::string(text), long_label);

    EXPECT_NO_FATAL_FAILURE(env->destroy_object(parent_id));
  }

  /// scene snapshot restore (editor undo/redo, play/stop) tears an object down through a
  ///  bare destroy_object on the parent's slot and rebuilds it under the same names; the
  ///  behavior slots and their managed objects must be released with the parent or the
  ///  rebuild collides with the leaked names and leaks the old instances
  TEST_F(script_environment_tests, destroy_parent_releases_behaviors_for_rebuild) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    for (int cycle = 0; cycle < 3; ++cycle) {
      integer_t parent_id = -1;
      EXPECT_NO_FATAL_FAILURE(parent_id = env->create_object("SnapshotParent"));
      ASSERT_GE(parent_id, 0);
      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(parent_id, "TestParentObject"));

      script_object* parent = env->get_object(parent_id);
      ASSERT_NE(parent, nullptr);
      ASSERT_NE(parent->dotnet_object, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_behavior(parent_id, "TestBehavior"));
      /// an empty handle list here means the managed name from the previous cycle leaked
      ///  and the behavior failed to instantiate
      ASSERT_EQ(parent->behavior_handles.size(), 1u) << "cycle " << cycle;

      const integer_t behavior_id = parent->behavior_handles[0].script_object_id;
      ASSERT_GE(behavior_id, 0) << "cycle " << cycle;
      script_object* behavior = env->get_object(behavior_id);
      ASSERT_NE(behavior, nullptr);
      EXPECT_NE(behavior->dotnet_object, nullptr) << "behavior slot should own its managed object";
      EXPECT_EQ(parent->dotnet_object->invoke<int>("CountBehaviors"), 1);

      /// scene teardown destroys only the parent's slot
      EXPECT_NO_FATAL_FAILURE(env->destroy_object(parent_id));
    }
  }

}  // namespace other