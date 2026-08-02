/**
 * \file tests/ref/ref_test.cpp
 **/
#include "ref_test.hpp"

namespace other {

  TEST_F(ref_test, default_constructor_creates_null_ref) {
    ref<test_ref_object<int32_t>> default_ref;
    ASSERT_EQ(default_ref.raw_ptr(), nullptr);
    ASSERT_EQ(default_ref.count(), 0);
    ASSERT_FALSE(default_ref);
  }

  TEST_F(ref_test, make_ref_counts_single_reference) {
    ref<test_ref_object<uint32_t>> ptr = make_ref<test_ref_object<uint32_t>>(42u);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);
    ASSERT_EQ(ptr->value, 42u);
  }

  TEST_F(ref_test, copy_constructor) {
    ref<test_ref_object<uint32_t>> ptr = make_ref<test_ref_object<uint32_t>>(42u);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);

    ASSERT_EQ(ptr->value, 42u);
    {
      ref<test_ref_object<uint32_t>> test_ref(ptr);
      ASSERT_NE(test_ref.raw_ptr(), nullptr);

      ASSERT_EQ(ptr.raw_ptr()->count(), 2);
      ASSERT_EQ(test_ref.raw_ptr()->count(), 2);

      ASSERT_EQ(test_ref->value, 42u);
    }
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
  }

  TEST_F(ref_test, move_constructor) {
    ref<test_ref_object<uint32_t>> ptr = make_ref<test_ref_object<uint32_t>>(42u);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);

    ref<test_ref_object<uint32_t>> test_ref(std::move(ptr));
    ASSERT_EQ(ptr.raw_ptr(), nullptr);
    ASSERT_NE(test_ref.raw_ptr(), nullptr);
    ASSERT_EQ(test_ref.raw_ptr()->count(), 1);
    ASSERT_EQ(test_ref->value, 42u);
  }

  TEST_F(ref_test, store_simple_container_type) {
    ref<test_ref_object<std::array<uint32_t, 4>>> container_ref = make_ref<test_ref_object<std::array<uint32_t, 4>>>(std::array<uint32_t, 4>{ 1, 2, 3, 4 });
    ASSERT_NE(container_ref.raw_ptr(), nullptr);
    ASSERT_EQ(container_ref.raw_ptr()->count(), 1);
    ASSERT_TRUE(container_ref);

    const auto& data = container_ref->value;
    ASSERT_EQ(data.size(), 4);
    ASSERT_EQ(data[0], 1);
    ASSERT_EQ(data[1], 2);
    ASSERT_EQ(data[2], 3);
    ASSERT_EQ(data[3], 4);
  }

  TEST_F(ref_test, inheritance_constructor) {
    ref<test_ref_object<std::array<int32_t, 4>>> base_ref = make_ref<my_thing>();
    ASSERT_NE(base_ref.raw_ptr(), nullptr);
    ASSERT_EQ(base_ref.raw_ptr()->count(), 1);
    ASSERT_TRUE(base_ref);
  }

  TEST_F(ref_test, last_release_runs_destructor) {
    bool destroyed = false;
    {
      ref<destruction_probe> probe = make_ref<destruction_probe>(&destroyed);
      ASSERT_EQ(probe.raw_ptr()->count(), 1);
      ASSERT_FALSE(destroyed);
    }
    ASSERT_TRUE(destroyed);
  }

  TEST_F(ref_test, destructor_runs_only_after_last_reference) {
    bool destroyed = false;
    ref<destruction_probe> outer;
    {
      ref<destruction_probe> inner = make_ref<destruction_probe>(&destroyed);
      outer = inner;
      ASSERT_EQ(outer.raw_ptr()->count(), 2);
    }
    ASSERT_FALSE(destroyed);
    ASSERT_EQ(outer.raw_ptr()->count(), 1);

    outer = nullptr;
    ASSERT_TRUE(destroyed);
  }

  TEST_F(ref_test, copy_assignment_releases_previous_referent) {
    bool first_destroyed = false;
    bool second_destroyed = false;

    ref<destruction_probe> target = make_ref<destruction_probe>(&first_destroyed);
    ref<destruction_probe> replacement = make_ref<destruction_probe>(&second_destroyed);

    target = replacement;
    ASSERT_TRUE(first_destroyed);
    ASSERT_FALSE(second_destroyed);
    ASSERT_EQ(target.raw_ptr(), replacement.raw_ptr());
    ASSERT_EQ(target.raw_ptr()->count(), 2);
  }

  TEST_F(ref_test, self_aliasing_copy_assignment_keeps_count_balanced) {
    bool destroyed = false;
    ref<destruction_probe> a = make_ref<destruction_probe>(&destroyed);
    ref<destruction_probe> b = a;
    ASSERT_EQ(a.raw_ptr()->count(), 2);

    a = b;
    ASSERT_FALSE(destroyed);
    ASSERT_EQ(a.raw_ptr()->count(), 2);

    a = nullptr;
    b = nullptr;
    ASSERT_TRUE(destroyed);
  }

  TEST_F(ref_test, move_assignment_releases_previous_referent) {
    bool first_destroyed = false;
    bool second_destroyed = false;

    ref<destruction_probe> target = make_ref<destruction_probe>(&first_destroyed);
    ref<destruction_probe> replacement = make_ref<destruction_probe>(&second_destroyed);

    target = std::move(replacement);
    ASSERT_TRUE(first_destroyed);
    ASSERT_FALSE(second_destroyed);
    ASSERT_EQ(replacement.raw_ptr(), nullptr);
    ASSERT_EQ(target.raw_ptr()->count(), 1);
  }

  TEST_F(ref_test, base_ref_release_runs_derived_destructor) {
    bool base_destroyed = false;
    bool derived_destroyed = false;
    {
      ref<destruction_probe> base_ref = make_ref<derived_probe>(&base_destroyed, &derived_destroyed);
      ASSERT_EQ(base_ref.raw_ptr()->count(), 1);
    }
    ASSERT_TRUE(derived_destroyed);
    ASSERT_TRUE(base_destroyed);
  }

}  // namespace other
