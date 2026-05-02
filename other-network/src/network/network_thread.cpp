/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/timer.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "network/message.hpp"

namespace other {

  void network_thread::on_initialize() {
    net_context = std::make_unique<network_context>();
    OTHER_ASSERT(net_context != nullptr, "Failed to create network context.");
    bus.register_thread();
  }

  void network_thread::on_start() {
    message network_ready_msg;
    network_ready_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_READY,
    };
    bus.send_message(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    net_context = nullptr;

    message shutdown_msg;
    shutdown_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_SHUTDOWN_COMPLETE,
    };
    bus.send_message(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    net_context->io_context.poll();

    auto msg = bus.receive_message(duration_cast<milliseconds>(tick_duration(10)));
    if (msg.has_value()) {
      switch (msg->header.category) {
        case CONTROL:
          switch (msg->header.id) {
            case PING: handle_control_ping(std::move(*msg)); break;
            // case PONG: handle_control_pong(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown CONTROL message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case COMMAND:
          switch (msg->header.id) {
            case SHUTDOWN_REQUEST: handle_command_shutdown_request(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            default:
              CORE_LOG_WARN("Network thread received unknown REQUEST message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        default:
          CORE_LOG_WARN("Network thread received unknown message category {}", static_cast<int>(msg->header.category));
          break;
      }
    }

    if (current_state.shutdown_pending) {
      if (current_state.shutdown_complete) {
        return;
      }

      message msg;
      msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };

      acknowledgement ackmsg;
      ackmsg.acked_header = {
        .category = COMMAND,
        .id = SHUTDOWN_REQUEST,
      };
      ackmsg.ack_nack = 1;
      msg.data.append_range(ackmsg.as_buffer());
      bus.send_message(std::move(msg));

      current_state.shutdown_complete = true;
    }
  }

  void network_thread::accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec) {
    if (ec && ec == asio::error::operation_aborted) {
      return;
    } else if (!ec && current_connections >= max_connections) {
      CORE_LOG_WARN("Maximum connections reached, rejecting new connection from {}", socket.remote_endpoint().address().to_string());
      socket.close();
      return;
    }
    CORE_LOG_DEBUG("Accepted new connection from {}", socket.remote_endpoint().address().to_string());
  }

  void network_thread::handle_control_ping(message&& msg) {
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");

    try {
      if (net_context->acceptor.is_open()) {
        net_context->acceptor.close();
      }
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error closing acceptor: {}", e.what());
    }

    current_state.shutdown_pending = true;
  }

}  // namespace other