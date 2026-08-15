/**
 * \file network_thread_tests.cpp
 **/
#include "network_thread_tests.hpp"

#include <atomic>

#include <gtest/gtest.h>

#include <mswsockdef.h>

#include "network/network_thread.hpp"
#include "network/transport_provider.hpp"
#include "network_thread_test_runner.hpp"

namespace other {

  namespace {

    /// plain counting fake: gmock expectations add noise the concurrency tests don't want
    struct counting_transport_provider : public socket_transport_provider {
      std::atomic<uint64_t> ticks = 0;

      std::string name() const override { return "CountingTransport"; }
      bool is_stream() const override { return true; }
      link_caps conn_caps(natural_t) const override { return {}; }
      void tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) override {}
      void net_close(natural_t connection_id) override {}

     private:
      void on_initialize() override {}
      void on_tick() override { ticks.fetch_add(1, std::memory_order_relaxed); }
      void on_shutdown() override {}
      void on_start_listen(natural_t conn_id, const binding_point& endpoint) override {}
      void on_start_connect(natural_t conn_id, const binding_point& endpoint) override {}
    };

    bool wait_until(const std::function<bool()>& pred, milliseconds deadline = milliseconds(2000)) {
      const auto start = steady_clock::now();
      while (!pred() && steady_clock::now() - start < deadline) {
        std::this_thread::sleep_for(milliseconds(1));
      }
      return pred();
    }

  }  // namespace

  bool network_thread_tests::wait_on_message(const message_header& header, message_bus& bus, microseconds timeout) {
    bool found = false;

    const milliseconds duration(500);
    auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      opt<message> msg_opt = bus.receive_message(microseconds(10));
      if (msg_opt.has_value()) {
        auto& msg = *msg_opt;
        if (message_header{ msg.category, msg.id } == header) {
          found = true;
          break;
        }
      }
      std::this_thread::sleep_for(milliseconds(100));
    }

    return found;
  }

  bool network_thread_tests::wait_on_acked_message(const message_header& original_header, message_bus& bus, microseconds timeout) {
    bool found = false;

    const milliseconds duration(500);
    auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      opt<message> msg_opt = bus.receive_message(microseconds(10));
      if (msg_opt.has_value()) {
        auto& msg = *msg_opt;
        if (message_header{ msg.category, msg.id } == message_header{ ACKNOWLEDGEMENT, ACK }) {
          auto ack_data = deserialize_direct<acknowledgement_ack>(msg.data).first;
          if (ack_data.acked_header == original_header) {
            found = true;
            break;
          }
        }
      }
      std::this_thread::sleep_for(milliseconds(100));
    }

    return found;
  }

  TEST_F(network_thread_tests, basic_verification) {
    message_bus bus;
    network_thread mock_thread{ bus };

    ASSERT_NO_FATAL_FAILURE(mock_thread.launch());

    const milliseconds duration(500);
    const auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      std::this_thread::sleep_for(milliseconds(100));
    }

    ASSERT_NO_FATAL_FAILURE(mock_thread.shutdown());
    ASSERT_NO_FATAL_FAILURE(mock_thread.wait_for_shutdown_complete());
  }

  TEST_F(network_thread_tests, verify_message_bus_communication) {
    message_bus bus;
    network_thread mock_thread{ bus };

    ASSERT_FALSE(bus.has_message());
    ASSERT_NO_FATAL_FAILURE(mock_thread.launch());

    ASSERT_TRUE(wait_on_message({ NOTIFICATION, NETWORK_THREAD_READY }, bus));

    {
      message shutdown_msg{ COMMAND, SHUTDOWN_REQUEST, {} };
      message req_ack_msg{ REQUEST, ACK, {} };
      request_acknowledgment req_ack_data{
        .ack_id = 1,
        .original_header = { shutdown_msg.category, shutdown_msg.id },
        .message_data = std::move(shutdown_msg.data),
      };
      ASSERT_NO_FATAL_FAILURE(req_ack_msg.data = serialize_direct(req_ack_data));
      ASSERT_NO_FATAL_FAILURE(bus.send_message(std::move(req_ack_msg)));
    }

    ASSERT_TRUE(wait_on_acked_message({ COMMAND, SHUTDOWN_REQUEST }, bus));

    ASSERT_NO_FATAL_FAILURE(mock_thread.shutdown());
    ASSERT_NO_FATAL_FAILURE(mock_thread.wait_for_shutdown_complete());

    ASSERT_TRUE(bus.has_message());
    {
      opt<message> msg_opt = bus.receive_message();
      ASSERT_TRUE(msg_opt.has_value());

      message msg = std::move(*msg_opt);
      EXPECT_EQ(msg.category, NOTIFICATION);
      EXPECT_EQ(msg.id, NETWORK_THREAD_SHUTDOWN_COMPLETE);
    }
  }

  TEST_F(network_thread_tests, reclamation_epoch_advances_while_pumping) {
    network_thread_test_runner test_runner;
    EXPECT_TRUE(test_runner.test_passes("reclamation epoch advances", [&](network_thread_test_context& context) {
      const uint64_t before = context.net_thread.reclamation_epoch();
      return wait_until([&]() { return context.net_thread.reclamation_epoch() > before + 1; });
    }));
  }

  TEST_F(network_thread_tests, unregistered_provider_stops_ticking_after_quiescence) {
    counting_transport_provider provider;

    network_thread_test_runner test_runner;
    EXPECT_TRUE(test_runner.test_passes("tombstone + epoch gate reclamation", [&](network_thread_test_context& context) {
      context.net_thread.register_provider(&provider);
      if (!wait_until([&]() { return provider.ticks.load(std::memory_order_relaxed) > 0; })) {
        return false;
      }

      /// the reclamation contract: tombstone, sample the epoch, and once it has advanced
      ///  past sample + 1 the pump provably no longer holds the pointer
      context.net_thread.unregister_provider(&provider);
      const uint64_t sampled = context.net_thread.reclamation_epoch();
      if (!wait_until([&]() { return context.net_thread.reclamation_epoch() > sampled + 1; })) {
        return false;
      }

      const uint64_t ticks_at_quiescence = provider.ticks.load(std::memory_order_relaxed);
      std::this_thread::sleep_for(milliseconds(50));
      EXPECT_EQ(provider.ticks.load(std::memory_order_relaxed), ticks_at_quiescence);
      return provider.ticks.load(std::memory_order_relaxed) == ticks_at_quiescence;
    }));
  }

}  // namespace other