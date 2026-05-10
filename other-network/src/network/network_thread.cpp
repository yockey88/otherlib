/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <cstdint>

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "thread/message.hpp"

#include "network/messages.hpp"
#include "network/network_error.hpp"
#include "network/transport_provider.hpp"

namespace other {

  void network_thread::register_provider(transport_provider* provider) {
    OTHER_ASSERT(provider != nullptr, "Cannot register null provider");
    std::lock_guard lock(providers_mutex);
    providers.push_back(provider);
  }

  transport_provider* network_thread::get_provider_by_name(const std::string& name) {
    std::lock_guard lock(providers_mutex);
    for (transport_provider* provider : providers) {
      if (provider->name() == name) {
        return provider;
      }
    }
    return nullptr;
  }

  void network_thread::register_connection_route(natural_t connection_id, transport_provider* provider, void* opaque_handle) {
    auto [itr, success] = active_connections.emplace(connection_id, connection_route{
                                                                      .provider = provider,
                                                                      .opaque_handle = opaque_handle,
                                                                    });
    OTHER_ASSERT(success, "Failed to register connection route for ID {}", connection_id);
  }

  void network_thread::register_listener_route(natural_t listener_id, transport_provider* provider, void* opaque_handle) {
    auto [itr, success] = active_listeners.emplace(listener_id, listener_route{
                                                                  .provider = provider,
                                                                  .opaque_handle = opaque_handle,
                                                                });
    OTHER_ASSERT(success, "Failed to register listener route for ID {}", listener_id);
  }

  void network_thread::mark_route_recently_closed(natural_t connection_id) {
    recently_closed_connections.push_back(connection_id);
  }

  void network_thread::send_to_driver(message&& msg) {
    CORE_LOG_TRACE("[NETWORK THREAD TX: {}]", msg.header);
    bus.send_message(std::move(msg));
  }

  void network_thread::on_initialize() {
    bus.register_thread();

    for (auto* p : providers) {
      OTHER_ASSERT(p != nullptr, "Provider list contains null provider");
      p->initialize(this, &network_io);
    }
  }

  void network_thread::on_start() {
    message network_ready_msg(NOTIFICATION, NETWORK_THREAD_READY);
    send_to_driver(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    for (auto* p : providers) {
      OTHER_ASSERT(p != nullptr, "Provider list contains null provider");
      p->shutdown();
    }
    providers.clear();

    message shutdown_msg(NOTIFICATION, NETWORK_THREAD_SHUTDOWN_COMPLETE);
    send_to_driver(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    if (current_state.shutdown_ready) {
      // we should allow user to re-open a connection with the same ID,
      // we should only actually close these on shutdown
      for (natural_t connection_id : recently_closed_connections) {
        active_connections.erase(connection_id);
        active_listeners.erase(connection_id);
      }
      recently_closed_connections.clear();

      current_state.shutdown_ready = active_connections.empty() && active_listeners.empty();
    }

    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    for (auto& provider : providers) {
      OTHER_ASSERT(provider != nullptr, "Provider list contains null provider");
      provider->tick();
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

      active_connections.clear();
      active_listeners.clear();

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
      CORE_LOG_TRACE("[NETWORK THREAD RX: {}]", msg->header);
      switch (msg->header.category) {
        case CONTROL:
          switch (msg->header.id) {
            case PING: handle_control_ping(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown CONTROL message ID {:#06x}", msg->header.id));
          }
          break;

        case COMMAND:
          switch (msg->header.id) {
            case SHUTDOWN_REQUEST: handle_command_shutdown_request(std::move(*msg)); break;
            case LISTEN_TCP_CONNECTION: handle_command_listen_tcp_connection(std::move(*msg)); break;
            case CONNECT_TCP_CONNECTION: handle_command_connect_tcp_connection(std::move(*msg)); break;
            case OPEN_UDP_CONNECTION: handle_command_open_udp_connection(std::move(*msg)); break;
            case CLOSE_TCP_CONNECTION: handle_command_close_tcp_connection(std::move(*msg)); break;
            case TX_DATA: handle_command_tx_data(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id));
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            case ACK: handle_request_ack_process_msg(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown REQUEST message ID {:#06x}", msg->header.id));
          }
          break;

        default:
          throw std::runtime_error(std::format("Network thread received message with unknown category {:#06x}", msg->header.category));
      }
    } else {
      /// no message received, just continue
    }
  }

  void network_thread::handle_control_ping(message&& msg) {
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");
    current_state.shutdown_pending = true;
    for (auto* provider : providers) {
      OTHER_ASSERT(provider != nullptr, "Provider list contains null provider");
      provider->begin_shutdown();
    }
  }

  /// \todo check for duplicate endpoints or other invalid connection parameters

  void network_thread::handle_command_listen_tcp_connection(message&& msg) {
    command_listen_tcp_connection request = deserialize_direct<command_listen_tcp_connection>(msg.data).first;

    transport_provider* provider = nullptr;
    for (auto* p : providers) {
      OTHER_ASSERT(p != nullptr, "Provider list contains null provider");
      if (FNV(p->name()) == request.transport_hash) {
        provider = p;
        break;
      }
    }

    if (provider == nullptr) {
      throw invalid_provider_network_error(std::format("No transport provider with {:#010x} to listen @ {}:{}", request.transport_hash, request.endpoint.ip, request.endpoint.port));
    }

    if (active_listeners.find(request.connection_id) != active_listeners.end()) {
      throw port_in_use_network_error(std::format("Listener with ID {} already exists for endpoint {}:{}", request.connection_id, request.endpoint.ip, request.endpoint.port));
    }

    provider->start_listen(request.endpoint);
  }

  void network_thread::handle_command_connect_tcp_connection(message&& msg) {
    // notif
    // binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    // asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    // natural_t connection_id = generate_connection_id();
    // auto [itr, success] = active_connections.emplace(connection_id, connection::create_tcp_connection(this, connection_id, events, network_io, endpoint, asio::ip::tcp::socket(network_io.context)));
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    //   return;
    // }

    // itr->second->
  }

  void network_thread::handle_command_open_udp_connection(message&& msg) {
    // binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    // asio::ip::udp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    // natural_t connection_id = generate_connection_id();
    // auto [itr, success] = active_connections.emplace(connection_id, connection::udp_connection(connection_id, events, network_io, endpoint));
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    //   return;
    // }
  }

  void network_thread::handle_command_close_tcp_connection(message&& msg) {
    // natural_t connection_id = *reinterpret_cast<const natural_t*>(msg.data.data());
    // auto itr = active_connections.find(connection_id);
    // if (itr == active_connections.end()) {
    //   CORE_LOG_WARN("Received request to close unknown connection ID {}", connection_id);
    //   return;
    // }

    // active_connections.erase(itr);
  }

  void network_thread::handle_command_tx_data(message&& msg) {
    command_tx_data request = deserialize_direct<command_tx_data>(msg.data).first;

    natural_t connection_id = request.connection_id;
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_WARN("Received request to send data on unknown connection ID {}", connection_id);
      return;
    }

    itr->second.provider->tx_data(connection_id, std::span<const uint8_t>(request.data.data(), request.data.size()));
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
      acked_msg.header = request_data.original_header;
      acked_msg.data = std::move(request_data.message_data);
    }

    uint8_t ack = 1;
    message_header original_header = acked_msg.header;
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

    if (immediately_acknowledge_message(acked_msg.header)) {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };
      acknowledgement_ack ack_data{
        .ack_id = ack_id,
        .acked_header = acked_msg.header,
        .ack = ack,
      };
      ack_msg.data = serialize_direct(ack_data);

      send_to_driver(std::move(ack_msg));
    } else {
      ack_list.add_pending_ack_response(ack_id, acked_msg.header);
    }
  }

}  // namespace other