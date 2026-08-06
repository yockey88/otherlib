/**
 * \file network/acknowledgement_list.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_ACKNOWLEDGEMENT_LIST_HPP
#define OTHER_NETWORK_NETWORK_ACKNOWLEDGEMENT_LIST_HPP

#include <asio/asio.hpp>

#include "message/message.hpp"

namespace other {

  struct acknowledgement_list {
    struct pending_ack {
      natural_t id = 0;

      message_header header;
      microseconds timeout_duration = microseconds(0);
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      message_handler handler;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_ack& other) const {
        return sent_time.time_since_epoch() <=> other.sent_time.time_since_epoch();
      }
    };

    natural_t register_ack(asio::io_context& io, message_header header, microseconds timeout, message_handler handler);
    void handle_ack(natural_t ack_id, message_header original_header, std::span<const uint8_t> data);
    void cancel_ack(natural_t ack_id);
    void clear();

    void add_pending_ack_response(natural_t ack_id, message_header header);
    natural_t get_pending_ack_response(message_header header);

   private:
    struct pending_response {
      natural_t ack_id;
      message_header header;
    };

    natural_t next_pending_ack_id = 1;
    std::deque<pending_ack> pending_acks;
    std::deque<pending_response> pending_responses;

    std::deque<natural_t> finish_ack_ids;

    inline natural_t generate_ack_id() { return next_pending_ack_id++; }
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_ACKNOWLEDGEMENT_LIST_HPP