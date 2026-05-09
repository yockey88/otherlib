/**
 * \file tests/scene/asset_tests.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_TESTS_HPP
#define OTHER_SCENE_ASSET_ASSET_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class asset_tests : public other_test {
   public:
    asio::io_context io_context;
    event_system events{ io_context };

    void SetUp() override {
    }
    void TearDown() override {
      io_context.stop();
    }
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_TESTS_HPP