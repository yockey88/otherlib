/**
 * \file tests/scripting/action_tests.cpp
 **/
#include "action_tests.hpp"

#include "core/defines.hpp"

#include "scripting/actions/action.hpp"

namespace other {

  TEST_F(action_tests, simple_action_execution) {
    action test_action("test_action", "A simple test action", make_ref<native_callback<void>>([]() {
                         CORE_LOG_DEBUG("Test action executed!");
                       }));

    ASSERT_NO_FATAL_FAILURE(test_action.execute<>());
  }

}  // namespace other