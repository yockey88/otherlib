/**
 * \file util/message_passing.hpp
 **/
#ifndef OTHER_UTIL_MESSAGE_PASSING_HPP
#define OTHER_UTIL_MESSAGE_PASSING_HPP

#include "thread/channel.hpp"
#include "thread/message.hpp"

namespace other {
  namespace signals {

    class bus {
     public:
      bus();

      bus(bus&&);
      bus& operator=(bus&&);

      bus(const bus&) = delete;
      bus& operator=(const bus&) = delete;

      void register_thread();

      opt<message> receive_message(std::chrono::microseconds timeout = std::chrono::microseconds(100));
      void send_message(message&& msg);

     private:
      struct thread_data {
        scope<channel<message>> tx_channel = nullptr;
        scope<channel<message>> rx_channel = nullptr;
        std::thread::id thread_id = std::this_thread::get_id();
      };

      /// two threads connected
      std::atomic<uint8_t> registered_thread_count = 0;
      std::mutex thread_data_mutex;
      thread_data threads[2];
    };

  }  // namespace signals
}  // namespace other

#endif  // OTHER_UTIL_MESSAGE_PASSING_HPP