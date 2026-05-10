/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <algorithm>
#include <cstdint>

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/network_error.hpp"
#include "network/transport_provider.hpp"

#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void network_thread::register_provider(transport_provider* provider) {
    OTHER_ASSERT(provider != nullptr, "Cannot register null provider");
    std::lock_guard lock(providers_mutex);
    providers.push_back(provider);
    CORE_LOG_DEBUG(" - network system registered transport provider '{}' ({:#010x})", provider->name(), provider->hash());
  }

  void network_thread::register_packet_sink(natural_t id, packet_sink* sink) {
    OTHER_ASSERT(sink != nullptr, "Cannot register null packet sink");
    std::lock_guard lock(sink_mutex);
    OTHER_ASSERT(std::ranges::find(packet_sinks, id, &target::id) == packet_sinks.end(), "Packet sink ID {} is already registered", id);

    packet_sinks.push_back({ id, sink });
    CORE_LOG_DEBUG(" - network system registered packet sink [{}] at address {:p}", id, static_cast<const void*>(sink));
  }

  void network_thread::register_transport_listener(natural_t transport_hash, natural_t id, packet_sink* sink) {
    OTHER_ASSERT(sink != nullptr, "Cannot register null packet sink");

    CORE_LOG_DEBUG("Registering transport listener for transport hash {:#010x} with packet sink ID {}", transport_hash, id);
    register_packet_sink(id, sink);

    {
      std::lock_guard lock(providers_mutex);
      auto provider_itr = std::ranges::find_if(providers, [transport_hash](transport_provider* p) { return p->hash() == transport_hash; });
      if (provider_itr == providers.end()) {
        CORE_LOG_ERROR("Failed to register transport listener: no provider found with hash {:#010x}", transport_hash);
        return;
      }

      transport_provider* provider = *provider_itr;
      OTHER_ASSERT(provider != nullptr, "Provider with hash {:#010x} is null", transport_hash);
      CORE_LOG_DEBUG(" - found provider '{}' for transport hash {:#010x}", provider->name(), transport_hash);
      provider->register_packet_sink(sink);
    }
  }

  void network_thread::attach_connection_listener(natural_t connection_id, natural_t id, packet_sink* sink) {
    OTHER_ASSERT(sink != nullptr, "Cannot register null packet sink");

    CORE_LOG_DEBUG("Attaching connection listener for connection ID {} with packet sink ID {}", connection_id, id);
    register_packet_sink(id, sink);

    auto conn = active_connections.find(connection_id);
    if (conn == active_connections.end()) {
      CORE_LOG_ERROR("Failed to attach connection listener: no active connection found with ID {}", connection_id);
      return;
    }

    conn->second.sink = sink;
  }

  void network_thread::attach_connection_listener(natural_t connection_id, natural_t sink_id) {
    if (std::ranges::find(packet_sinks, sink_id, &target::id) == packet_sinks.end()) {
      CORE_LOG_ERROR("Failed to attach connection listener: no packet sink found with ID {}", sink_id);
      return;
    }

    auto conn = active_connections.find(connection_id);
    if (conn == active_connections.end()) {
      CORE_LOG_ERROR("Failed to attach connection listener: no active connection found with ID {}", connection_id);
      return;
    }

    conn->second.sink = std::ranges::find(packet_sinks, sink_id, &target::id)->sink;
  }

  void network_thread::register_connection_route(natural_t connection_id, transport_provider* provider, void* opaque_handle) {
    auto [itr, success] = active_connections.emplace(connection_id, connection_route{
                                                                      .provider = provider,
                                                                      .opaque_handle = opaque_handle,
                                                                    });
    OTHER_ASSERT(success, "Failed to register connection route for ID {}", connection_id);
    CORE_LOG_DEBUG("Registered connection route for ID {} with provider '{}'", connection_id, provider->name());
  }

  void network_thread::register_listener_route(natural_t listener_id, transport_provider* provider, void* opaque_handle) {
    auto [itr, success] = active_listeners.emplace(listener_id, listener_route{
                                                                  .provider = provider,
                                                                  .opaque_handle = opaque_handle,
                                                                });
    OTHER_ASSERT(success, "Failed to register listener route for ID {}", listener_id);
    CORE_LOG_DEBUG("Registered listener route for ID {} with provider '{}'", listener_id, provider->name());
  }

  void network_thread::mark_route_recently_closed(natural_t connection_id) {
    recently_closed_connections.push_back(connection_id);
  }

  void network_thread::send_to_driver(message&& msg) {
    CORE_LOG_TRACE("[NETWORK THREAD TX: {}]", message_header{ msg.category, msg.id });
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
    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    if (current_state.shutdown_pending) {
      // we should allow user to re-open a connection with the same ID,
      // we should only actually close these on shutdown
      for (natural_t connection_id : recently_closed_connections) {
        CORE_LOG_DEBUG("Cleaning up connection ID {}", connection_id);
        for (auto& provider : providers) {
          OTHER_ASSERT(provider != nullptr, "Provider list contains null provider");
          provider->connection_removed(connection_id);
        }

        active_connections.erase(connection_id);
        active_listeners.erase(connection_id);
      }
      recently_closed_connections.clear();

      const bool connections_shutdown = active_connections.empty();
      const bool listeners_shutdown = active_listeners.empty();
      current_state.shutdown_ready = connections_shutdown && listeners_shutdown;
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
      CORE_LOG_TRACE("[NETWORK THREAD RX: {}]", message_header{ msg->category, msg->id });
      switch (msg->category) {
        case CONTROL:
          switch (msg->id) {
            case PING: handle_control_ping(std::move(*msg)); break;
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

  void network_thread::handle_command_listen_connection(message&& msg) {
    command_listen_connection request = deserialize_direct<command_listen_connection>(msg.data).first;

    transport_provider* provider = nullptr;
    for (auto* p : providers) {
      OTHER_ASSERT(p != nullptr, "Provider list contains null provider");
      if (p->hash() == request.transport_hash) {
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

    provider->start_listen(request.connection_id, request.endpoint);
  }

  void network_thread::handle_command_connect_connection(message&& msg) {
  }

  void network_thread::handle_command_close_connection(message&& msg) {
    command_close_connection request = deserialize_direct<command_close_connection>(msg.data).first;

    natural_t connection_id = request.connection_id;
    auto conn_itr = active_connections.find(connection_id);
    if (conn_itr == active_connections.end()) {
      CORE_LOG_WARN("Received request to close unknown connection ID {}", connection_id);
      return;
    }

    transport_provider* provider = conn_itr->second.provider;
    provider->close(connection_id);
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