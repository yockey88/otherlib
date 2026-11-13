/**
 * \file tests/scene/asset_tests.hpp
 **/
#ifndef OTHER_TESTS_SCENE_ASSET_TESTS_HPP
#define OTHER_TESTS_SCENE_ASSET_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class asset_tests : public other_test {
   public:
    asio::io_context io_context;
  };

}  // namespace other

#endif  // OTHER_TESTS_SCENE_ASSET_TESTS_HPP