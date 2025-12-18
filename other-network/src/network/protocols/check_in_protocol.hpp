/**
 * \file network/protocols/check_in_protocol.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_PROTOCOLS_CHECK_IN_PROTOCOL_HPP
#define OTHER_NETWORK_NETWORK_PROTOCOLS_CHECK_IN_PROTOCOL_HPP

#include "network/session_protocol_handler.hpp"

namespace other {

  message_sequence create_check_in_sequence(bool is_server);

  class server_check_in_handler : public protocol_handler {
   public:
    server_check_in_handler(session* s)
        : protocol_handler(s, session_protocol_data{ session_protocol::SESSION_SERVER_CHECK_IN, std::vector<message_sequence>{ create_check_in_sequence(true) } }) {}
    virtual ~server_check_in_handler() = default;

   protected:
    integer_t received_session_id = -1;

    void on_protocol_completion() override;
    void transmit_current_message() override;

    void handle_control_ping(const message_header& header, const std::span<uint8_t> data) override;
  };

  class client_check_in_handler : public protocol_handler {
   public:
    client_check_in_handler(session* s)
        : protocol_handler(s, session_protocol_data{ session_protocol::SESSION_CLIENT_CHECK_IN, std::vector<message_sequence>{ create_check_in_sequence(false) } }) {}
    virtual ~client_check_in_handler() = default;

   protected:
    void on_protocol_completion() override;
    void transmit_current_message() override;

    void handle_request_session_check_in(const message_header& header, const std::span<uint8_t> data) override;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_PROTOCOLS_CHECK_IN_PROTOCOL_HPP