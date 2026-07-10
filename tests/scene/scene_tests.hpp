/**
 * \file test/scene/scene_test.hpp
 **/
#ifndef OTHER_TESTS_SCENE_SCENE_TESTS_HPP
#define OTHER_TESTS_SCENE_SCENE_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class scene_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }
  };

}  // namespace other

#endif  // OTHER_TESTS_SCENE_SCENE_TESTS_HPP