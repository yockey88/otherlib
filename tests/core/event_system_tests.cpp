/**
 * \file tests/core/event_system_tests.cpp
 *
 * contract under test: recurring timed events re-arm a single timer entry
 *  instead of accumulating one dead entry per period.
 **/
#include "core/event_system_tests.hpp"

namespace other {

  TEST_F(event_system_tests, recurring_timer_reuses_single_entry) {
    asio::io_context io;
    event_system events{ io };

    int32_t fires = 0;
    events.register_timed_event("test.recurring-tick", std::chrono::milliseconds(1), true);
    events.add_listener("test.recurring-tick", [&fires](const value&) { fires++; });

    ASSERT_TRUE(pump_until(io, [&fires]() { return fires >= 5; })) << "recurring event never fired enough times";

    /// however many times it fired, a recurring event owns exactly one timer entry
    ASSERT_GE(fires, 5);
    ASSERT_EQ(events.active_timer_count(), 1);

    events.cancel_event("test.recurring-tick");
    ASSERT_EQ(events.active_timer_count(), 0);
    ASSERT_FALSE(events.has_event("test.recurring-tick"));
  }

  TEST_F(event_system_tests, one_shot_timer_cleans_up_after_firing) {
    asio::io_context io;
    event_system events{ io };

    int32_t fires = 0;
    events.register_timed_event("test.one-shot", std::chrono::milliseconds(1), false);
    events.add_listener("test.one-shot", [&fires](const value&) { fires++; });

    ASSERT_TRUE(pump_until(io, [&fires]() { return fires >= 1; })) << "one-shot event never fired";

    /// firing a non-recurring event cancels it: no timer entry, no registration, no refire
    ASSERT_TRUE(pump_until(io, [&events]() { return events.active_timer_count() == 0; }));
    ASSERT_EQ(fires, 1);
    ASSERT_FALSE(events.has_event("test.one-shot"));
  }

}  // namespace other
