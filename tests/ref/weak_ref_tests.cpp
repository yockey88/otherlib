/**
 * \file tests/ref/weak_ref_tests.cpp
 *
 * contract under test (see core/weak_ref.hpp):
 *  - a weak_ref does NOT keep the object logically alive: once the last strong ref
 *    releases, expired() is true and lock() returns null (no resurrection)
 *  - a weak_ref DOES pin the storage: the destructor is deferred until the last
 *    weak_ref releases, so observing expiry is never a use-after-free
 **/
#include "ref/weak_ref_tests.hpp"

namespace other {

  TEST_F(weak_ref_tests, default_weak_is_expired) {
    weak_ref<destruction_probe> weak;
    ASSERT_TRUE(weak.expired());
    ASSERT_FALSE(weak);
    ASSERT_EQ(weak.lock(), nullptr);
    ASSERT_TRUE(weak == nullptr);
  }

  TEST_F(weak_ref_tests, weak_observes_expiry_without_extending_lifetime) {
    bool destroyed = false;
    weak_ref<destruction_probe> weak;
    {
      ref<destruction_probe> strong = make_ref<destruction_probe>(&destroyed);
      weak = weak_ref(strong);
      ASSERT_FALSE(weak.expired());
      ASSERT_TRUE(weak);
      ASSERT_EQ(strong.count(), 1);
    }

    /// the last strong ref is gone: logically dead even though the weak_ref pins storage
    ASSERT_TRUE(weak.expired());
    ASSERT_EQ(weak.lock(), nullptr);
    ASSERT_FALSE(destroyed);

    weak = nullptr;
    ASSERT_TRUE(destroyed);
  }

  TEST_F(weak_ref_tests, lock_provides_a_real_strong_reference) {
    bool destroyed = false;
    {
      weak_ref<destruction_probe> weak;
      {
        ref<destruction_probe> strong = make_ref<destruction_probe>(&destroyed);
        weak = weak_ref(strong);

        ref<destruction_probe> locked = weak.lock();
        ASSERT_NE(locked, nullptr);
        ASSERT_EQ(strong.count(), 2);
        ASSERT_NE(locked->destroyed, nullptr);
      }
      /// both the original and the locked ref released inside the scope
      ASSERT_TRUE(weak.expired());
      ASSERT_FALSE(destroyed);
    }
    ASSERT_TRUE(destroyed);
  }

  TEST_F(weak_ref_tests, locked_reference_outlives_the_original_strong_ref) {
    bool destroyed = false;
    ref<destruction_probe> strong = make_ref<destruction_probe>(&destroyed);
    weak_ref<destruction_probe> weak(strong);

    ref<destruction_probe> locked = weak.lock();
    strong = nullptr;

    /// the locked ref alone keeps the object alive
    ASSERT_FALSE(weak.expired());
    ASSERT_FALSE(destroyed);
    ASSERT_EQ(locked.count(), 1);

    locked = nullptr;
    ASSERT_TRUE(weak.expired());
    ASSERT_EQ(weak.lock(), nullptr);
  }

  TEST_F(weak_ref_tests, copies_and_aliasing_assignment_stay_balanced) {
    bool destroyed = false;
    ref<destruction_probe> strong = make_ref<destruction_probe>(&destroyed);

    weak_ref<destruction_probe> w1(strong);
    weak_ref<destruction_probe> w2(w1);
    weak_ref<destruction_probe> w3;
    w3 = w1;
    /// aliasing assignment: both already observe the same object
    w2 = w3;

    strong = nullptr;
    ASSERT_TRUE(w1.expired());
    ASSERT_TRUE(w2.expired());
    ASSERT_TRUE(w3.expired());
    ASSERT_FALSE(destroyed);

    w1 = nullptr;
    w2 = nullptr;
    ASSERT_FALSE(destroyed);
    w3 = nullptr;
    ASSERT_TRUE(destroyed);
  }

  TEST_F(weak_ref_tests, move_transfers_observation) {
    bool destroyed = false;
    ref<destruction_probe> strong = make_ref<destruction_probe>(&destroyed);

    weak_ref<destruction_probe> w1(strong);
    weak_ref<destruction_probe> w2(std::move(w1));
    ASSERT_TRUE(w1 == nullptr);
    ASSERT_FALSE(w2.expired());

    weak_ref<destruction_probe> w3;
    w3 = std::move(w2);
    ASSERT_TRUE(w2 == nullptr);
    ASSERT_FALSE(w3.expired());

    strong = nullptr;
    ASSERT_FALSE(destroyed);
    w3 = nullptr;
    ASSERT_TRUE(destroyed);
  }

}  // namespace other
