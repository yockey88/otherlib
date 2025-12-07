/**
 * \file network/protocols/check_in_protocol.cpp
 **/
#include "network/protocols/check_in_protocol.hpp"

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
    if (current_msg_matches(CONTROL, PONG)) {
      message pong_msg;
      pong_msg.header = {
        .category = CONTROL,
        .id = PONG,
      };
      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&get_session().session_id);
      pong_msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));

      get_session().start_write(std::move(pong_msg));
    } else if (current_msg_matches(REQUEST, SESSION_CHECK_IN)) {
      message check_in_msg;
      check_in_msg.header = {
        .category = REQUEST,
        .id = SESSION_CHECK_IN,
      };
      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&get_session().session_id);
      check_in_msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));

      get_session().start_write(std::move(check_in_msg));
    } else {
      OTHER_ASSERT(false, "Unhandled transmit message in server check-in handler : {}", current_msg.header);
    }
  }

  void server_check_in_handler::handle_control_ping(const message_header& header, const std::span<uint8_t> data) {
    if (data.size() < sizeof(integer_t)) {
      throw network_packet_parse_error("Invalid PING message data size");
    }

    integer_t received_session_id = *reinterpret_cast<const integer_t*>(data.data());
    if (get_session().session_id != session::kInvalidSessionId && received_session_id != session::kInvalidSessionId && get_session().session_id != received_session_id) {
      CORE_LOG_ERROR(" - Launch check in session ID mismatch: local {}, received {}", get_session().session_id, received_session_id);
      get_session().thread->report_connection_closed(get_session().connection_id, get_session().session_id);
      return;
    }

    bool create_new_id = get_session().session_id == session::kInvalidSessionId && received_session_id == session::kInvalidSessionId;
    if (create_new_id) {
      CORE_LOG_TRACE(" - Launch check in requires new session ID");
      /// get new session id and seend it back after the PONG
      get_session().session_id = get_session().thread->get_next_connection_id();
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

      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&get_session().session_id);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      /// other session related data

      OTHER_ASSERT(msg.data.size() < session::kBufferSize, "Check-in message data is too large");
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
      const uint8_t* acked_header_bytes = reinterpret_cast<const uint8_t*>(&acked_header);
      ack_msg.data.append_range(std::span(acked_header_bytes, sizeof(message_header)));

      get_session().start_write(std::move(ack_msg));
    } else {
      OTHER_ASSERT(false, "Unhandled transmit message in client check-in handler : {}", get_current_message().header);
    }
  }

  void client_check_in_handler::handle_request_session_check_in(const message_header& header, const std::span<uint8_t> data) {
    CORE_LOG_DEBUG("Client received SESSION_CHECK_IN request");
    const integer_t received_session_id = *reinterpret_cast<const integer_t*>(data.data());
    if (get_session().session_id == session::kInvalidSessionId && received_session_id != session::kInvalidSessionId) {
      CORE_LOG_DEBUG(" - Launch check in adopting session ID {}", received_session_id);
      get_session().session_id = received_session_id;
    }
    /// otherwise see if they are both invalid
    else if (get_session().session_id == session::kInvalidSessionId && received_session_id == session::kInvalidSessionId) {
      /// \todo: send back msg either 1 - need id message or 2 - generate an id and send 'suggest-this-id' message
    }
    /// otherwise see if they are both valid but different
    else if (get_session().session_id != session::kInvalidSessionId && received_session_id != session::kInvalidSessionId && get_session().session_id != received_session_id) {
      CORE_LOG_ERROR(" - Launch check in session ID mismatch: local {}, received {}", get_session().session_id, received_session_id);
      /// \todo maybe we send back something to clarify?
      get_session().thread->report_connection_closed(get_session().connection_id, get_session().session_id);
    }
  }

}  // namespace other