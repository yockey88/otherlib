/**
 * \file message/message_bus.cpp
 **/
#include "message/message_bus.hpp"

#include <cstdint>

#include "core/profiler.hpp"

namespace other {

  message_bus::message_bus() {
    PROFILE_SECTION("message_bus::message_bus");
    ref<channel_queue<message>> tx_queue = make_ref<channel_queue<message>>();
    ref<channel_queue<message>> rx_queue = make_ref<channel_queue<message>>();

    auto [this_tx_channel, thread_rx_channel] = message_channel::make_channel(tx_queue);
    auto [thread_tx_channel, this_rx_channel] = message_channel::make_channel(rx_queue);

    threads[0].tx_channel = std::move(this_tx_channel);
    threads[0].rx_channel = std::move(this_rx_channel);

    threads[1].tx_channel = std::move(thread_tx_channel);
    threads[1].rx_channel = std::move(thread_rx_channel);
  }

  message_bus::message_bus(message_bus&& other) {
    std::lock_guard lock(other.thread_data_mutex);
    threads[0] = std::move(other.threads[0]);
    threads[1] = std::move(other.threads[1]);
  }

  message_bus& message_bus::operator=(message_bus&& other) {
    if (this != &other) {
      std::lock_guard lock(other.thread_data_mutex);
      threads[0] = std::move(other.threads[0]);
      threads[1] = std::move(other.threads[1]);
    }
    return *this;
  }

  void message_bus::register_thread() {
    std::lock_guard lock(thread_data_mutex);
    OTHER_ASSERT(registered_thread_count < 2, "Cannot register more than two threads to message message_bus");
    uint8_t index = registered_thread_count++;

    thread_data& td = threads[index];
    td.thread_id = std::this_thread::get_id();
  }

  uint8_t message_bus::current_thread_index() {
    std::lock_guard lock(thread_data_mutex);
    if (threads[0].thread_id == std::this_thread::get_id()) {
      return 0;
    }
    if (threads[1].thread_id == std::this_thread::get_id()) {
      return 1;
    }
    OTHER_ASSERT(false, "Thread not registered with message message_bus");
    return 0;
  }

  opt<message> message_bus::receive_message(microseconds timeout) {
    PROFILE_SECTION("message_bus::receive_message");
    return threads[current_thread_index()].rx_channel->await_message(timeout);
  }

  opt<message> message_bus::try_receive_message() {
    PROFILE_SECTION("message_bus::try_receive_message");
    return threads[current_thread_index()].rx_channel->try_pop();
  }

  void message_bus::send_message(message&& msg) {
    PROFILE_SECTION("message_bus::send_message");
    threads[current_thread_index()].tx_channel->push(std::move(msg));
  }

  bool message_bus::has_message() {
    PROFILE_SECTION("message_bus::has_message");
    return threads[current_thread_index()].rx_channel->empty() == false;
  }

}  // namespace other