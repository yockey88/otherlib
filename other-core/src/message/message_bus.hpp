/**
 * \file message/message_bus.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGE_BUS_HPP
#define OTHER_CORE_MESSAGE_MESSAGE_BUS_HPP

#include "message/message.hpp"

namespace other {

  class message_bus {
   public:
    message_bus();

    message_bus(message_bus&&);
    message_bus& operator=(message_bus&&);

    message_bus(const message_bus&) = delete;
    message_bus& operator=(const message_bus&) = delete;

    void register_thread();

    opt<message> receive_message(microseconds timeout = microseconds(10));
    void send_message(message&& msg);

    bool has_message();

   private:
    struct thread_data {
      scope<message_channel> tx_channel = nullptr;
      scope<message_channel> rx_channel = nullptr;
      std::thread::id thread_id = std::this_thread::get_id();
    };

    /// two threads connected
    std::atomic<uint8_t> registered_thread_count = 0;
    std::mutex thread_data_mutex;
    thread_data threads[2];
  };

}  // namespace other

#endif  // OTHER_CORE_MESSAGE_MESSAGE_BUS_HPP