/**
 * \file tests/thread/thread_tests.cpp
 **/
#include "thread/thread_tests.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "thread/message.hpp"
#include "thread/thread.hpp"

namespace other {

  MATCHER(IsLaunchingOrWaiting, "") {
    /// could be any of the following depending on what the computer is doing
    return arg == thread::LAUNCHING ||
      arg == thread::WAITING ||
      arg == thread::PROCESSING ||
      arg == thread::PUMPING;
  }

  class thread_test_thread : public thread {
   public:
    thread_test_thread()
        : thread("Thread-Test-Thread") {}
    virtual ~thread_test_thread() = default;

    MOCK_METHOD(void, pump_thread, (), (override));
    MOCK_METHOD(void, on_initialize, (), (override));
    MOCK_METHOD(void, on_start, (), (override));
    MOCK_METHOD(void, on_shutdown, (), (override));
  };

  TEST_F(thread_tests, thread_launch_and_shutdown) {
    thread_test_thread test_thread;

    EXPECT_CALL(test_thread, on_initialize()).Times(1);
    EXPECT_CALL(test_thread, on_start()).Times(1);
    EXPECT_CALL(test_thread, on_shutdown()).Times(1);
    EXPECT_CALL(test_thread, pump_thread()).Times(testing::AtLeast(1));

    // EXPECT_CALL(test_thread, handle_ping(testing::_))
    //   .Times(1)
    //   .WillOnce([](const session_status_request& ping) {
    //     // Do nothing
    //     EXPECT_EQ(ping.session_type, 1);
    //     EXPECT_EQ(ping.node_id, 42);
    //   });

    thread* thread_ptr = &test_thread;

    thread_ptr->launch();
    /// let it get to the waiting state
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_THAT(thread_ptr->get_current_state(), IsLaunchingOrWaiting());

    // session_status_request ping_msg;
    // ping_msg.session_type = 1;
    // ping_msg.node_id = 42;
    // message ping_message(CONTROL, PING);
    // ping_message.data = ping_msg.build();
    // thread_ptr->send_message(std::move(ping_message));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // signals stop
    thread_ptr->shutdown();
    // blocks until thread has fully stopped
    thread_ptr->wait_for_shutdown_complete();

    EXPECT_EQ(thread_ptr->get_current_state(), thread::STOPPED);
  }

  class thread_test_unjoinable_thread : public thread {
   public:
    thread_test_unjoinable_thread()
        : thread("Thread-Test-Unjoinable-Thread") {}
    virtual ~thread_test_unjoinable_thread() = default;

    MOCK_METHOD(void, pump_thread, (), (override));
    MOCK_METHOD(void, on_initialize, (), (override));
    MOCK_METHOD(void, on_start, (), (override));
    // Note: on_shutdown is not mocked to simulate unjoinable behavior
  };

  // TEST_F(thread_tests, thread_force_shutdown) {
  //   thread_test_thread test_thread;

  //   EXPECT_CALL(test_thread, on_initialize()).Times(1);
  //   EXPECT_CALL(test_thread, on_start()).Times(1);
  //   EXPECT_CALL(test_thread, on_shutdown()).Times(0);  /// should not be called
  //   EXPECT_CALL(test_thread, pump_thread()).Times(testing::AtLeast(1));

  //   thread* thread_ptr = &test_thread;

  //   thread_ptr->launch();
  //   /// let it get to the waiting state
  //   std::this_thread::sleep_for(std::chrono::milliseconds(100));

  //   thread_ptr->force_shutdown();
  //   EXPECT_EQ(thread_ptr->get_current_state(), thread::STOPPED);
  // }

}  // namespace other