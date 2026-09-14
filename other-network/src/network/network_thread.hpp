/**
 * \file network/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP

#include <asio/asio.hpp>

#include "thread/slot_registry.hpp"
#include "thread/thread.hpp"

#include "network/acknowledgement_list.hpp"
#include "network/io.hpp"

#include "message/message_bus.hpp"
#include "message/message_fields.hpp"

namespace other {

  class OTHER_CLASS network_thread : public thread {
   public:
    struct target {
      natural_t id = 0;
      // packet_sink* sink = nullptr;
    };
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), network_io{} {}
    virtual ~network_thread() = default;

    inline uint64_t reclamation_epoch() const {
      return pump_epoch.load(std::memory_order_seq_cst);
    }

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
    std::atomic<uint64_t> pump_epoch = 0;
    std::atomic<size_t> route_count = 0;
    // ostd::vector<transport_provider*> providers;
    ostd::vector<target> packet_sinks;

    acknowledgement_list ack_list;

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;
    void process_message(opt<message>&& msg);

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