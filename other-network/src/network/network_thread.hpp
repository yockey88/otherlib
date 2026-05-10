/**
 * \file network/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP

#include <asio/asio.hpp>

#include "thread/thread.hpp"

#include "network/acknowledgement_list.hpp"
#include "network/connection_route.hpp"
#include "network/io.hpp"
#include "network/listener_route.hpp"

#include "message/message_bus.hpp"
#include "message/message_fields.hpp"

#include "peer-mesh/packet_sink.hpp"

namespace other {

  class transport_provider;

  class network_thread : public thread {
   public:
    struct target {
      natural_t id = 0;
      packet_sink* sink = nullptr;
    };
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), network_io{} {}
    virtual ~network_thread() = default;

    inline natural_t generate_connection_id() {
      natural_t new_id = connection_id_counter.fetch_add(1, std::memory_order_relaxed);
      CORE_LOG_TRACE("[NEW CONN ID: {}]", new_id);
      return new_id;
    }

    void register_provider(transport_provider* provider);
    void register_packet_sink(natural_t id, packet_sink* sink);

    void register_transport_listener(natural_t transport_hash, natural_t id, packet_sink* sink);
    void attach_connection_listener(natural_t connection_id, natural_t id, packet_sink* sink);
    void attach_connection_listener(natural_t connection_id, natural_t sink_id);

    void register_connection_route(natural_t connection_id, transport_provider* provider, void* opaque_handle);
    void register_listener_route(natural_t listener_id, transport_provider* provider, void* opaque_handle);
    void mark_route_recently_closed(natural_t connection_id);

    void send_to_driver(message&& msg);

    inline bool is_shutdown_pending() const { return current_state.shutdown_pending; }
    inline message_bus& get_message_bus() { return bus; }
    inline asio::io_context& get_io_context() { return network_io.context; }

   private:
    struct state {
      std::mutex mutex;

      natural_t shutdown_ack_id = 0;
      bool shutdown_pending = false;
      bool shutdown_ready = false;
      bool shutdown_complete = false;
    };

    message_bus& bus;
    state current_state{};
    io network_io;

    std::atomic<natural_t> connection_id_counter = 1;

    std::map<natural_t, connection_route> active_connections;
    std::map<natural_t, listener_route> active_listeners;
    std::deque<natural_t> recently_closed_connections;

    std::mutex providers_mutex;
    std::mutex sink_mutex;
    std::vector<transport_provider*> providers;
    std::vector<target> packet_sinks;

    acknowledgement_list ack_list;

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;
    void process_message(opt<message>&& msg);

    void handle_control_ping(message&& msg);
    void handle_command_shutdown_request(message&& msg);
    void handle_command_listen_connection(message&& msg);
    void handle_command_connect_connection(message&& msg);
    void handle_command_close_connection(message&& msg);
    void handle_command_tx_data(message&& msg);

    bool immediately_acknowledge_message(const message_header& header);
    void handle_request_ack_process_msg(message&& msg);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;

    microseconds get_message_timeout() override {
      return microseconds(10);
    }
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_THREAD_HPP
