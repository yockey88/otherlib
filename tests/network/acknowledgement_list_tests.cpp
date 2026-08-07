/**
 * \file network/acknowledgement_list_tests.cpp
 **/
#include <gtest/gtest.h>

#include "network/acknowledgement_list.hpp"

#include "other_test.hpp"

namespace other {

  class acknowledgement_list_tests : public other_test {
   protected:
    /// pump the io_context until pred() or the deadline; asio timers only fire when run
    template <typename Pred>
    bool run_io_until(asio::io_context& io, Pred&& pred, milliseconds deadline = milliseconds(1000)) {
      const auto start = steady_clock::now();
      while (!pred() && steady_clock::now() - start < deadline) {
        io.run_for(milliseconds(10));
        if (io.stopped()) {
          io.restart();
        }
      }
      return pred();
    }
  };

  TEST_F(acknowledgement_list_tests, timeout_invokes_handler_and_reaps_entry) {
    asio::io_context io;
    acknowledgement_list acks;

    uint32_t timeouts = 0;
    message_handler handler{
      nullptr,
      [&](message_header header) { ++timeouts; },
    };
    natural_t id = acks.register_ack(io, { COMMAND, LISTEN_CONNECTION }, milliseconds(20), handler);
    EXPECT_NE(id, 0u);
    EXPECT_EQ(acks.pending_count(), 1u);

    EXPECT_TRUE(run_io_until(io, [&]() { return timeouts == 1; }));
    EXPECT_EQ(acks.pending_count(), 0u);
  }

  TEST_F(acknowledgement_list_tests, timeout_with_null_handler_still_reaps) {
    asio::io_context io;
    acknowledgement_list acks;

    acks.register_ack(io, { COMMAND, LISTEN_CONNECTION }, milliseconds(20), message_handler{});
    EXPECT_EQ(acks.pending_count(), 1u);

    /// the leak regression: a timed-out entry with no on_timeout must not linger forever
    EXPECT_TRUE(run_io_until(io, [&]() { return acks.pending_count() == 0; }));
  }

  TEST_F(acknowledgement_list_tests, handle_ack_runs_handler_and_cancels_timeout) {
    asio::io_context io;
    acknowledgement_list acks;

    uint32_t handled = 0;
    uint32_t timeouts = 0;
    message_handler handler{
      [&](message_header header, const std::span<const uint8_t> data) { ++handled; },
      [&](message_header header) { ++timeouts; },
    };
    const message_header header{ COMMAND, LISTEN_CONNECTION };
    natural_t id = acks.register_ack(io, header, milliseconds(50), handler);

    acks.handle_ack(id, header, {});
    EXPECT_EQ(handled, 1u);
    EXPECT_EQ(acks.pending_count(), 0u);

    /// run past the original deadline: the cancelled timer must not fire the timeout path
    run_io_until(io, [&]() { return false; }, milliseconds(120));
    EXPECT_EQ(timeouts, 0u);
  }

  TEST_F(acknowledgement_list_tests, middle_erase_leaves_other_timers_armed) {
    asio::io_context io;
    acknowledgement_list acks;

    uint32_t timeouts = 0;
    message_handler handler{
      nullptr,
      [&](message_header header) { ++timeouts; },
    };
    const message_header header{ COMMAND, LISTEN_CONNECTION };
    natural_t first = acks.register_ack(io, header, milliseconds(30), handler);
    natural_t second = acks.register_ack(io, header, milliseconds(30), handler);
    natural_t third = acks.register_ack(io, header, milliseconds(30), handler);
    EXPECT_NE(first, second);
    EXPECT_NE(second, third);

    /// erasing from the middle used to move the later entries' timers, cancelling their
    ///  pending waits — both survivors must still reach their timeout handlers
    acks.handle_ack(second, header, {});
    EXPECT_EQ(acks.pending_count(), 2u);

    EXPECT_TRUE(run_io_until(io, [&]() { return timeouts == 2; }));
    EXPECT_EQ(acks.pending_count(), 0u);
  }

  TEST_F(acknowledgement_list_tests, pending_response_returns_stored_ack_id) {
    acknowledgement_list acks;
    const message_header header{ COMMAND, SHUTDOWN_REQUEST };

    acks.add_pending_ack_response(42, header);
    /// the regression: this returned header.id instead of the registered ack id
    EXPECT_EQ(acks.get_pending_ack_response(header), 42u);
    EXPECT_EQ(acks.get_pending_ack_response(header), 0u);
  }

  TEST_F(acknowledgement_list_tests, clear_cancels_everything) {
    asio::io_context io;
    acknowledgement_list acks;

    uint32_t timeouts = 0;
    message_handler handler{
      nullptr,
      [&](message_header header) { ++timeouts; },
    };
    acks.register_ack(io, { COMMAND, LISTEN_CONNECTION }, milliseconds(20), handler);
    acks.register_ack(io, { COMMAND, LISTEN_CONNECTION }, milliseconds(20), handler);
    acks.clear();
    EXPECT_EQ(acks.pending_count(), 0u);

    run_io_until(io, [&]() { return false; }, milliseconds(80));
    EXPECT_EQ(timeouts, 0u);
  }

}  // namespace other
