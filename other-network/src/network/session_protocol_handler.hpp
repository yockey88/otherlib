/**
 * \file network/session_protocol_handler.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP
#define OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP

#include "thread/message.hpp"

namespace other {

  class session;

  enum class session_protocol {
    SESSION_SERVER_CHECK_IN,
    SESSION_CLIENT_CHECK_IN,
    SESSION_SHUTDOWN,
  };

  struct message_sequence {
    struct message {
      enum {
        NONE = -1,
        RX = 0,
        TX = 1,
      } rx_tx;
      message_header header;
      natural_t count;
    };

    std::vector<message> messages;
  };

  struct session_protocol_data {
    session_protocol function;
    std::vector<message_sequence> sequences;
  };

  class protocol_handler {
   public:
    protocol_handler(session* s, const session_protocol_data& data)
        : session_ptr(s), protocol_data(data) {}
    virtual ~protocol_handler() = default;

    session_protocol get_protocol_id() const {
      return protocol_data.function;
    }

    void force_set_sequence_index(natural_t index, natural_t message_idx);

    bool poll();

   protected:
    bool current_msg_matches(const message_header& header);
    bool current_msg_matches(uint16_t category, uint16_t id) {
      return current_msg_matches(message_header{ .category = category, .id = id });
    }

    session& get_session();

    message_sequence& get_current_sequence();
    message_sequence::message& get_current_message();

    bool receive_current_message();

    void complete_protocol();
    virtual void on_protocol_completion() {}

    virtual void transmit_current_message() = 0;

    virtual void handle_acknowledgement_ack(const message_header& header, const std::span<uint8_t> data) {}

    virtual void handle_control_ping(const message_header& header, const std::span<uint8_t> data) {}
    virtual void handle_control_pong(const message_header& header, const std::span<uint8_t> data) {}

    virtual void handle_request_session_check_in(const message_header& header, const std::span<uint8_t> data) {}

   private:
    session* session_ptr = nullptr;
    session_protocol_data protocol_data{};

    natural_t sequence_index = 0;
    natural_t message_index = 0;
    natural_t message_count_index = 0;

    void process_message(message&& msg);
    void increment_sequence_index();
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP