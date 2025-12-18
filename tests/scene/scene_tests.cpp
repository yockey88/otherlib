/**
 * \file tests/scene/scene_tests.cpp
 **/
#include "scene_tests.hpp"

#include <map>

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
    scene scene1("Original Scene");
    scene_object& obj1 = scene1.create_object("Object in Original Scene");
    natural_t obj1_id = obj1.id;

    scene scene2{ std::move(scene1) };

    EXPECT_EQ(scene2.name, "Original Scene");
    EXPECT_EQ(scene2.get_object_count(), 1u);

    scene_object& moved_obj = scene2.get_object(obj1_id);
    EXPECT_EQ(moved_obj.name, "Object in Original Scene");
  }

  TEST_F(scene_tests, scene_move_assignment) {
    scene scene2;
    natural_t obj1_id = 0;

    // this time ensure destructor is called on previous storage
    {
      scene scene1("Original Scene");
      scene_object& obj1 = scene1.create_object("Object in Original Scene");
      obj1_id = obj1.id;

      scene2 = std::move(scene1);
    }

    EXPECT_EQ(scene2.name, "Original Scene");
    EXPECT_EQ(scene2.get_object_count(), 1u);

    scene_object& moved_obj = scene2.get_object(obj1_id);
    EXPECT_EQ(moved_obj.name, "Object in Original Scene");
  }

  TEST_F(scene_tests, scene_move_and_still_works) {
    scene scene1("Original Scene");
    scene_object& obj1 = scene1.create_object("Object in Original Scene");
    natural_t obj1_id = obj1.id;

    scene scene2 = std::move(scene1);

    EXPECT_EQ(scene2.name, "Original Scene");
    EXPECT_EQ(scene2.get_object_count(), 1u);

    scene_object& moved_obj = scene2.get_object(obj1_id);
    EXPECT_EQ(moved_obj.name, "Object in Original Scene");

    scene_object& new_obj = scene2.create_object("New Object in Moved Scene");
    EXPECT_EQ(new_obj.name, "New Object in Moved Scene");
  }

  TEST_F(scene_tests, use_scene_from_std_map) {
    std::map<natural_t, scene> scene_map;
    {
      scene scene1("Mapped Scene");
      scene_object& obj1 = scene1.create_object("Object in Mapped Scene");
      natural_t scene_id = scene1.id;
      natural_t obj1_id = obj1.id;

      scene_map.emplace(scene_id, std::move(scene1));
    }

    auto it = scene_map.begin();
    EXPECT_NE(it, scene_map.end());

    scene& mapped_scene = it->second;
    EXPECT_EQ(mapped_scene.name, "Mapped Scene");
    EXPECT_EQ(mapped_scene.get_object_count(), 1u);

    scene_object& mapped_obj = mapped_scene.get_object(it->second.get_all_object_ids().front());
    EXPECT_EQ(mapped_obj.name, "Object in Mapped Scene");
  }

  TEST_F(scene_tests, use_scene_from_std_map2) {
    std::unordered_map<natural_t, scene> scene_map;
    /// this forces a weird constructor order edge case that left the registry in the storage
    ///    referencing the a scene that was destructed bc this constructs a temp and moves it into the map instead of constructing in place
    auto [itr, success] = scene_map.emplace(0, scene("Mapped Scene"));
    ASSERT_TRUE(success);
    ASSERT_TRUE(itr != scene_map.end());

    scene& mapped_scene = itr->second;
    EXPECT_EQ(mapped_scene.name, "Mapped Scene");
    EXPECT_EQ(mapped_scene.id, FNV("Mapped Scene"));

    scene_object& obj1 = itr->second.create_object("Object in Mapped Scene");
    natural_t obj1_id = obj1.id;
  }

  TEST_F(scene_tests, use_scene_from_std_map3) {
    std::unordered_map<natural_t, scene> scene_map;
    /// this forces a weird constructor order edge case that left the registry in the storage
    ///    referencing the a scene that was destructed bc this constructs a temp and moves it into the map instead of constructing in place
    auto [itr, success] = scene_map.emplace(0, "Mapped Scene");
    ASSERT_TRUE(success);
    ASSERT_TRUE(itr != scene_map.end());

    scene& mapped_scene = itr->second;
    EXPECT_EQ(mapped_scene.name, "Mapped Scene");
    EXPECT_EQ(mapped_scene.id, FNV("Mapped Scene"));

    scene_object& obj1 = itr->second.create_object("Object in Mapped Scene");
    natural_t obj1_id = obj1.id;
  }

  TEST_F(scene_tests, use_scene_from_std_vector) {
    std::vector<scene> scene_vector;
    scene& scene = scene_vector.emplace_back("Vector Scene");
    EXPECT_EQ(scene.name, "Vector Scene");
    EXPECT_EQ(scene.id, FNV("Vector Scene"));

    scene_object& obj1 = scene.create_object("Object in Vector Scene");
    natural_t obj1_id = obj1.id;
  }

}  // namespace other