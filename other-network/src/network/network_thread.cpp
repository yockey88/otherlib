/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <algorithm>
#include <cstdint>

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/enum_formatter.hpp"
#include "core/profiler.hpp"
#include "core/time.hpp"

#include "network/network_error.hpp"

#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void network_thread::send_to_driver(message&& msg) {
    CORE_LOG_TRACE("[NETWORK THREAD TX: {}]", message_header{ msg.category, msg.id });
    bus.send_message(std::move(msg));
  }

  void network_thread::on_initialize() {
    bus.register_thread();
  }

  void network_thread::on_start() {
    message network_ready_msg(NOTIFICATION, NETWORK_THREAD_READY);
    send_to_driver(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    message shutdown_msg(NOTIFICATION, NETWORK_THREAD_SHUTDOWN_COMPLETE);
    send_to_driver(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    if (current_state.shutdown_pending) {
      current_state.shutdown_ready = true;
      // connections_shutdown && listeners_shutdown;
    }

    auto msg = bus.receive_message(microseconds(1));
    try {
      process_message(std::move(msg));
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error processing message in network thread: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("Unknown error processing message in network thread");
    }

    if (current_state.shutdown_ready) {
      if (current_state.shutdown_complete) {
        return;
      }

      natural_t ack_response_id = ack_list.get_pending_ack_response({ COMMAND, SHUTDOWN_REQUEST });
      if (ack_response_id != 0) {
        message ack_msg(ACKNOWLEDGEMENT, ACK);
        acknowledgement_ack ack_data{
          .ack_id = ack_response_id,
          .acked_header = { COMMAND, SHUTDOWN_REQUEST },
          .ack = 1,
        };
        ack_msg.data = serialize_direct(ack_data);
        send_to_driver(std::move(ack_msg));
      }

      current_state.shutdown_complete = true;
      CORE_LOG_DEBUG("Network thread shutdown complete");
    }
  }

  void network_thread::process_message(opt<message>&& msg) {
    if (msg.has_value()) {
      CORE_LOG_TRACE("[NETWORK THREAD RX: {}]", message_header{ msg->category, msg->id });
      switch (msg->category) {
        case CONTROL:
          switch (msg->id) {
            default:
              throw std::runtime_error(std::format("Network thread received unknown CONTROL message ID {:#06x}", msg->id));
          }
          break;

        case COMMAND:
          switch (msg->id) {
            case SHUTDOWN_REQUEST: handle_command_shutdown_request(std::move(*msg)); break;
            case LISTEN_CONNECTION: handle_command_listen_connection(std::move(*msg)); break;
            case CONNECT_CONNECTION: handle_command_connect_connection(std::move(*msg)); break;
            case TX_DATA: handle_command_tx_data(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown COMMAND message ID {:#06x}", msg->id));
          }
          break;

        case REQUEST:
          switch (msg->id) {
            case ACK: handle_request_ack_process_msg(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown REQUEST message ID {:#06x}", msg->id));
          }
          break;

        default:
          throw std::runtime_error(std::format("Network thread received message with unknown category {:#06x}", msg->category));
      }
    } else {
      /// no message received, just continue
    }
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");
    current_state.shutdown_pending = true;
  }

  /// \todo check for duplicate endpoints or other invalid connection parameters

  void network_thread::handle_command_listen_connection(message&& msg) {
    command_listen_connection request = deserialize_direct<command_listen_connection>(msg.data).first;
  }

  void network_thread::handle_command_connect_connection(message&& msg) {
  }

  void network_thread::handle_command_close_connection(message&& msg) {
    command_close_connection request = deserialize_direct<command_close_connection>(msg.data).first;
  }

  void network_thread::handle_command_tx_data(message&& msg) {
    command_tx_data request = deserialize_direct<command_tx_data>(msg.data).first;
  }

  bool network_thread::immediately_acknowledge_message(const message_header& header) {
    if (header == message_header{ CONTROL, SHUTDOWN_REQUEST }) {
      return false;
    }
    return true;
  }

  void network_thread::handle_request_ack_process_msg(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(natural_t) + sizeof(message_header), "Invalid ACK message data size: {}", msg.data.size());
    natural_t ack_id = 0;
    message acked_msg;

    {
      request_acknowledgment request_data = deserialize_direct<request_acknowledgment>(msg.data).first;
      ack_id = request_data.ack_id;
      acked_msg.category = request_data.original_header.category;
      acked_msg.id = request_data.original_header.id;
      acked_msg.data = std::move(request_data.message_data);
    }

    uint8_t ack = 1;
    message_header original_header = { acked_msg.category, acked_msg.id };
    try {
      process_message(std::move(acked_msg));
    } catch (const network_error& e) {
      CORE_LOG_ERROR("Error handler invoked: [{}]", original_header);
      CORE_LOG_ERROR("Failed to process message in network thread: {}", e.what());
      ack = 0;
    } catch (const std::runtime_error& e) {
      CORE_LOG_ERROR("Runtime error handling acknowledgment for message {}: {}", original_header, e.what());
      ack = 0;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error handling acknowledgment for message {}: {}", original_header, e.what());
      ack = 0;
    } catch (...) {
      CORE_LOG_ERROR("Unknown error handling acknowledgment for message {}", original_header);
      ack = 0;
    }

    if (immediately_acknowledge_message({ acked_msg.category, acked_msg.id })) {
      message ack_msg(ACKNOWLEDGEMENT, ACK);
      acknowledgement_ack ack_data{
        .ack_id = ack_id,
        .acked_header = { acked_msg.category, acked_msg.id },
        .ack = ack,
      };
      ack_msg.data = serialize_direct(ack_data);

      send_to_driver(std::move(ack_msg));
    } else {
      ack_list.add_pending_ack_response(ack_id, { acked_msg.category, acked_msg.id });
    }
  }

}  // namespace other