/**
 * \file network/protocols/check_in_protocol.cpp
 **/
#include "network/protocols/check_in_protocol.hpp"

#include "network/message.hpp"
#include "network/network_thread.hpp"
#include "network/session.hpp"

namespace other {

  message_sequence create_check_in_sequence(bool is_server) {
    auto mseq = message_sequence{
      {
        { message_sequence::message::NONE, { CONTROL, PING }, 1 },
        { message_sequence::message::NONE, { CONTROL, PONG }, 1 },
        { message_sequence::message::NONE, { REQUEST, SESSION_CHECK_IN }, 1 },
        { message_sequence::message::NONE, { ACKNOWLEDGEMENT, ACK }, 1 },
      }
    };

    if (is_server) {
      auto& msgs = mseq.messages;
      msgs[0].rx_tx = message_sequence::message::RX;
      msgs[1].rx_tx = message_sequence::message::TX;
      msgs[2].rx_tx = message_sequence::message::TX;
      msgs[3].rx_tx = message_sequence::message::RX;
    } else {
      auto& msgs = mseq.messages;
      msgs[0].rx_tx = message_sequence::message::TX;
      msgs[1].rx_tx = message_sequence::message::RX;
      msgs[2].rx_tx = message_sequence::message::RX;
      msgs[3].rx_tx = message_sequence::message::TX;
    }
    return mseq;
  }

  void server_check_in_handler::on_protocol_completion() {
    get_session().checked_in();
    get_session().set_timeout(seconds(10), false, &session::on_heartbeat_timeout);
  }

  void server_check_in_handler::transmit_current_message() {
    auto& current_msg = get_current_message();

    /// respond to PING with the session id they told us
    if (current_msg_matches(CONTROL, PONG)) {
      message pong_msg;
      pong_msg.header = {
        .category = CONTROL,
        .id = PONG,
      };
      control_pong pong_msg_data;
      pong_msg_data.session_id = received_session_id;
      pong_msg.data.append_range(pong_msg_data.as_buffer());

      get_session().start_write(std::move(pong_msg));
    }
    /// then request them to check in with our session id
    else if (current_msg_matches(REQUEST, SESSION_CHECK_IN)) {
      message check_in_msg;
      check_in_msg.header = {
        .category = REQUEST,
        .id = SESSION_CHECK_IN,
      };
      session_check_in_request out_msg;
      out_msg.session_id = get_session().session_id;
      check_in_msg.data.append_range(out_msg.as_buffer());

      get_session().start_write(std::move(check_in_msg));
    } else {
      OTHER_ASSERT(false, "Unhandled transmit message in server check-in handler : {}", current_msg.header);
    }
  }

  void server_check_in_handler::handle_control_ping(const message_header& header, const std::span<uint8_t> data) {
    control_ping ping_msg = other_message_spec::parse<control_ping>(data);
    received_session_id = ping_msg.session_id;
    /// case 1 session has id, client does not
    if (get_session().session_id != session::kInvalidSessionId && received_session_id == session::kInvalidSessionId) {
      CORE_LOG_DEBUG("Overriding local session ID {}", get_session().session_id);
    }
    /// case 2 session has id, client contains a match
    else if (get_session().session_id != session::kInvalidSessionId && received_session_id != session::kInvalidSessionId && get_session().session_id == received_session_id) {
      CORE_LOG_DEBUG("Session ID {} matches remote", get_session().session_id);
    }
    /// case 3 session has id, client contains a different id
    else if (get_session().session_id != session::kInvalidSessionId && received_session_id != session::kInvalidSessionId && get_session().session_id != received_session_id) {
      CORE_LOG_DEBUG("Overriding remote session ID {} with local ID {}", received_session_id, get_session().session_id);
    }
    /// case 4 session has no id, client has no id
    else if (get_session().session_id == session::kInvalidSessionId && received_session_id == session::kInvalidSessionId) {
      /// we will need to generate a new session id later
      CORE_LOG_DEBUG("Both local and remote session IDs are invalid, generating a new session ID");
      get_session().session_id = get_session().thread->get_next_session_id();
      CORE_LOG_DEBUG("  - Assigned new session ID {}", get_session().session_id);
    }
    /// case 5 server has not id and client has an id (and it is unique)
    else if (get_session().session_id == session::kInvalidSessionId && received_session_id != session::kInvalidSessionId
             /// \todo check it is unique
    ) {
      CORE_LOG_DEBUG("Adopting remote session ID {}", received_session_id);
      get_session().session_id = received_session_id;
    }
    /// case 6 server has no id, client has an id (and it the server knows another session with that id and the addresses match)
    else if (get_session().session_id == session::kInvalidSessionId && received_session_id != session::kInvalidSessionId
             /// \todo check if another session exists with that id
    ) {
      /// \todo ... for now we will just adopt it
      CORE_LOG_DEBUG("Local ID is invalid, and remote is already in use, generating new session ID instead of adopting {}", received_session_id);
      get_session().session_id = get_session().thread->get_next_session_id();
      CORE_LOG_DEBUG("  - Assigned new session ID {}", get_session().session_id);
    } else {
      OTHER_ASSERT(false, "Unhandled session ID check-in case: local id={}, remote id={}", get_session().session_id, received_session_id);
    }
  }

  void client_check_in_handler::on_protocol_completion() {
    get_session().checked_in();
  }

  void client_check_in_handler::transmit_current_message() {
    if (current_msg_matches(CONTROL, PING)) {
      message msg;
      msg.header = {
        .category = CONTROL,
        .id = PING,
      };

      control_ping ping_msg;
      ping_msg.session_id = get_session().session_id;

      /// \todo add timestamp
      // ping_msg.timesamp = ...
      msg.data.append_range(ping_msg.as_buffer());

      get_session().dump_message_bytes(msg);
      get_session().start_write(std::move(msg));
    } else if (current_msg_matches(ACKNOWLEDGEMENT, ACK)) {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };
      message_header acked_header = {
        .category = REQUEST,
        .id = SESSION_CHECK_IN,
      };

      acknowledgement ack;
      ack.acked_header = acked_header;
      ack.ack_nack = get_ack_byte();
      ack_msg.data.append_range(ack.as_buffer());

      get_session().start_write(std::move(ack_msg));
    } else {
      OTHER_ASSERT(false, "Unhandled transmit message in client check-in handler : {}", get_current_message().header);
    }
  }

  void client_check_in_handler::handle_request_session_check_in(const message_header& header, const std::span<uint8_t> data) {
    CORE_LOG_DEBUG("Client received SESSION_CHECK_IN request");
    const integer_t received_session_id = *reinterpret_cast<const integer_t*>(data.data());
    if (received_session_id != session::kInvalidSessionId && get_session().session_id != received_session_id) {
      CORE_LOG_DEBUG(" - Session adopting ID from check-in {}", received_session_id);
      get_session().session_id = received_session_id;
    }
    /// otherwise see if they are both invalid
    else if (get_session().session_id == session::kInvalidSessionId && received_session_id == session::kInvalidSessionId) {
      /// \todo: send back msg either 1 - need id message or 2 - generate an id and send 'suggest-this-id' message
      CORE_LOG_ERROR("Unimplemented: both client and server have invalid session IDs during check-in");
    }
  }

}  // namespace other