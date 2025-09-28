/**
 * \file tests/scene/scene_tests.cpp
 **/
#include "scene_tests.hpp"

#include "scene/scene.hpp"

namespace other {

  TEST_F(scene_tests, create_scene_object) {
    scene scene1;

    scene_object& obj1 = scene1.create_object("Test Object 1");
    EXPECT_EQ(obj1.name, "Test Object 1");
    EXPECT_EQ(obj1.id, 1u);

    scene_object& obj2 = scene1.create_object("Test Object 2", &obj1);
    EXPECT_EQ(obj2.name, "Test Object 2");
    EXPECT_GT(obj2.id, 0);

    scene_object* parent_of_obj2 = scene1.get_parent(obj2.id);
    EXPECT_NE(parent_of_obj2, nullptr);
    EXPECT_EQ(parent_of_obj2->id, obj1.id);
  }

  TEST_F(scene_tests, scene_move_constructor) {
    scene scene2;
    natural_t obj1_id = 0;
    {
      scene scene1("Original Scene");
      scene_object& obj1 = scene1.create_object("Object in Original Scene");
      obj1_id = obj1.id;

      scene2 = { std::move(scene1) };
    }

    EXPECT_EQ(scene2.name, "Original Scene");
    EXPECT_EQ(scene2.get_object_count(), 1u);

    scene_object& moved_obj = scene2.get_object(obj1_id);
    EXPECT_EQ(moved_obj.name, "Object in Original Scene");
  }

}  // namespace other