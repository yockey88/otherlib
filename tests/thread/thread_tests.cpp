/**
 * \file tests/thread/thread_tests.cpp
 **/
#include "thread/thread_tests.hpp"

#include "thread/thread.hpp"

namespace other {

  TEST_F(thread_tests, thread_launch_and_shutdown) {
    thread test_thread("Test Thread");

    test_thread.launch();
    /// let it get to the waiting state
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(test_thread.get_current_state(), thread::WAITING);

    test_thread.shutdown();
    EXPECT_EQ(test_thread.get_current_state(), thread::STOPPED);
  }

}  // namespace other