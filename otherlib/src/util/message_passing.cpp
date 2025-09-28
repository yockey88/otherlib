/**
 * \file util/message_passing.cpp
 **/
#include "util/message_passing.hpp"

#include <cstdint>

namespace other {
  namespace signals {

    bus::bus() {
      ref<channel_queue<message>> tx_queue = make_ref<channel_queue<message>>();
      ref<channel_queue<message>> rx_queue = make_ref<channel_queue<message>>();

      auto [this_tx_channel, thread_rx_channel] = channel<message>::make_channel(tx_queue);
      auto [thread_tx_channel, this_rx_channel] = channel<message>::make_channel(rx_queue);

      threads[0].tx_channel = std::move(this_tx_channel);
      threads[0].rx_channel = std::move(this_rx_channel);

      threads[1].tx_channel = std::move(thread_tx_channel);
      threads[1].rx_channel = std::move(thread_rx_channel);
    }

    bus::bus(bus&& other) {
      std::lock_guard lock(other.thread_data_mutex);
      threads[0] = std::move(other.threads[0]);
      threads[1] = std::move(other.threads[1]);
    }

    bus& bus::operator=(bus&& other) {
      if (this != &other) {
        std::lock_guard lock(other.thread_data_mutex);
        threads[0] = std::move(other.threads[0]);
        threads[1] = std::move(other.threads[1]);
      }
      return *this;
    }

    void bus::register_thread() {
      std::lock_guard lock(thread_data_mutex);
      OTHER_ASSERT(registered_thread_count < 2, "Cannot register more than two threads to message bus");
      uint8_t index = registered_thread_count++;

      thread_data& td = threads[index];
      td.thread_id = std::this_thread::get_id();
    }

    opt<message> bus::receive_message(std::chrono::microseconds timeout) {
      uint8_t index = 0;
      {
        std::lock_guard lock(thread_data_mutex);
        if (threads[0].thread_id == std::this_thread::get_id()) {
          index = 0;
        } else if (threads[1].thread_id == std::this_thread::get_id()) {
          index = 1;
        } else {
          OTHER_ASSERT(false, "Thread not registered with message bus");
        }
      }

      return threads[index].rx_channel->await_message(timeout);
    }

    void bus::send_message(message&& msg) {
      uint8_t index = 0;
      {
        std::lock_guard lock(thread_data_mutex);
        if (threads[0].thread_id == std::this_thread::get_id()) {
          index = 0;
        } else if (threads[1].thread_id == std::this_thread::get_id()) {
          index = 1;
        } else {
          OTHER_ASSERT(false, "Thread not registered with message bus");
        }
      }

      threads[index].tx_channel->push(std::move(msg));
    }

  }  // namespace signals
}  // namespace other