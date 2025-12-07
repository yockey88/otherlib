/**
 * \file network/session_protocol_handler.cpp
 **/
#include "network/session_protocol_handler.hpp"

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "network/network_thread.hpp"
#include "network/session.hpp"

namespace other {

  void protocol_handler::force_set_sequence_index(natural_t index, natural_t message_idx) {
    sequence_index = index;
    message_index = message_idx;
  }

  bool protocol_handler::poll() {
    if (sequence_index >= protocol_data.sequences.size()) {
      CORE_LOG_TRACE("Protocol handler ({} steps) completing at sequence index {}, message index {}", protocol_data.sequences.size(), sequence_index, message_index);
      return true;
    }

    message_sequence::message& current_msg = get_current_message();
    bool next_message = false;

    if (current_msg.rx_tx == message_sequence::message::RX) {
      if (receive_current_message()) {
        next_message = true;
      }
    } else if (current_msg.rx_tx == message_sequence::message::TX) {
      transmit_current_message();
      get_session().start_read();
      next_message = true;
    } else {
      OTHER_ASSERT(false, "Invalid RX/TX value in protocol handler message");
    }

    if (next_message) {
      increment_sequence_index();
    }

    bool complete = sequence_index >= protocol_data.sequences.size();
    if (complete) {
      complete_protocol();
    }
    return complete;
  }

  bool protocol_handler::current_msg_matches(const message_header& header) {
    return header == get_current_message().header;
  }

  session& protocol_handler::get_session() {
    OTHER_ASSERT(session_ptr != nullptr, "Session pointer is null");
    return *session_ptr;
  }

  message_sequence& protocol_handler::get_current_sequence() {
    OTHER_ASSERT(sequence_index < protocol_data.sequences.size(), "Sequence index out of range");
    return protocol_data.sequences[sequence_index];
  }

  message_sequence::message& protocol_handler::get_current_message() {
    auto& sequence = get_current_sequence();
    OTHER_ASSERT(message_index < sequence.messages.size(), "Message index out of range");
    return sequence.messages[message_index];
  }

  bool protocol_handler::receive_current_message() {
    message_sequence::message& current_msg = get_current_message();
    CORE_LOG_TRACE("Protocol handler expecting to receive message {}", current_msg.header);
    if (current_msg.rx_tx != message_sequence::message::RX) {
      OTHER_ASSERT(false, "Current message is not a receive message");
    }

    opt<message> opt_msg = get_session().receive_next_message();
    if (!opt_msg.has_value()) {
      return false;
    }

    message msg = std::move(opt_msg.value());
    process_message(std::move(msg));
    return true;
  }

  void protocol_handler::complete_protocol() {
    CORE_LOG_DEBUG("Protocol handler [{}] completed", get_protocol_id());
    on_protocol_completion();
  }

  void protocol_handler::process_message(message&& msg) {
    message_sequence::message& current_msg = get_current_message();
    if (msg.header != current_msg.header) {
      CORE_LOG_WARN("Received unexpected message {} when expecting {}", msg.header, current_msg.header);
      return;
    }
    CORE_LOG_DEBUG("Processing message [{}] for protocol [{}]", msg.header, get_protocol_id());

    switch (msg.header.category) {
      case ACKNOWLEDGEMENT:
        switch (msg.header.id) {
          case ACK: handle_acknowledgement_ack(msg.header, msg.data); break;
          default:
            CORE_LOG_WARN("Unhandled ACKNOWLEDGEMENT message ID {:#06x} in protocol handler", msg.header.id);
            break;
        }
        break;

      case CONTROL:
        switch (msg.header.id) {
          case PING: handle_control_ping(msg.header, msg.data); break;
          case PONG: handle_control_pong(msg.header, msg.data); break;
          default:
            CORE_LOG_WARN("Unhandled CONTROL message ID {:#06x} in protocol handler", msg.header.id);
            break;
        }
        break;

      case REQUEST:
        switch (msg.header.id) {
          case SESSION_CHECK_IN: handle_request_session_check_in(msg.header, msg.data); break;
          default:
            CORE_LOG_WARN("Unhandled REQUEST message ID {:#06x} in protocol handler", msg.header.id);
            break;
        }
        break;

      default:
        CORE_LOG_WARN("Unhandled message category {} in protocol handler", msg.header.category);
        break;
    }
  }

  void protocol_handler::increment_sequence_index() {
    message_count_index++;
    if (message_count_index >= get_current_message().count) {
      message_count_index = 0;

      message_index++;
      if (message_index >= get_current_sequence().messages.size()) {
        message_index = 0;

        sequence_index++;
      }
    }
    if (sequence_index < protocol_data.sequences.size()) {
      CORE_LOG_TRACE("Session protocol sequence index: {},{} [{} count: {}]", sequence_index, message_index, get_current_message().header, message_count_index);
    }
  }

}  // namespace other