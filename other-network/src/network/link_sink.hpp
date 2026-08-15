/**
 * \file network/link_sink.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_LINK_SINK_HPP
#define OTHER_NETWORK_NETWORK_LINK_SINK_HPP

#include <deque>
#include <functional>
#include <mutex>
#include <span>

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/frame.hpp"
#include "network/link.hpp"
#include "network/link_security.hpp"
#include "network/packet_sink.hpp"

namespace other {

  class transport_provider;
  enum class mesh_message : uint16_t;

  /// THE link: created by its provider at connection establishment, adopted by a mesh.
  ///  owns the record, framing, link control (hello/keepalive/bye/auth), security
  ///  transforms, and the one ordered handoff queue between the provider's home thread
  ///  and the mesh. pre-adoption it is a connection-shaped shell that buffers events
  class link_sink final : public packet_sink {
   public:
    struct settings {
      natural_t app_hash = 0;
      uint32_t max_frame_size = kDefaultMaxFrameSize;
      microseconds handshake_timeout{ 3'000'000 };
      microseconds keepalive_idle{ 5'000'000 };
      microseconds link_timeout{ 15'000'000 };
    };

    /// main-thread hooks bound at adoption; frame payloads arrive post-decrypt,
    ///  control frames never arrive at all
    struct events {
      std::function<void(link_record&)> link_up;
      std::function<void(const link_record&, link_close_reason)> link_down;
      std::function<void(const link_record&, uint16_t net_id, std::span<const uint8_t> payload)> frame;
    };

    link_sink(transport_provider& provider, natural_t conn_id);

    /// packet_sink surface — the provider's home thread. main-home providers deliver
    ///  straight through; net-home providers enqueue for pump()
    void rx_data(natural_t conn_id, std::span<const uint8_t> data) override;
    void connection_opened(natural_t conn_id) override;
    void connection_closed(natural_t conn_id) override;

    /// mesh surface — main thread
    void adopt(natural_t link_id, node_id local_seat, link_state initial, const settings& config,
               link_security* security, mesh_counters* counters, events hooks, microseconds now);
    bool adopted() const { return record.link_id != 0; }
    natural_t conn_id() const { return connection; }
    transport_provider& provider() { return home; }
    link_record& link() { return record; }
    const link_record& link() const { return record; }

    /// drains queued events + runs handshake/keepalive/timeout duties
    void pump(microseconds now);
    /// app-frame tx: encrypt, size limit, frame, move. payload bytes are opaque;
    ///  control-page refusal happens upstream
    bool tx(uint16_t net_id, std::span<const uint8_t> payload);
    /// link_security's channel while AUTHENTICATING
    void send_auth(std::span<const uint8_t> blob);
    void begin_close(link_close_reason reason, bool send_bye);

   private:
    struct rx_event {
      enum class kind : uint8_t { OPENED, BYTES, CLOSED };
      kind k = kind::BYTES;
      ostd::vector<uint8_t> bytes;
    };

    transport_provider& home;
    natural_t connection = 0;
    bool inline_delivery = false;
    bool stream = false;

    std::mutex queue_mutex;
    std::deque<rx_event> queue;

    link_record record;
    frame_reader reader;
    settings cfg;
    link_security* sec = nullptr;
    mesh_counters* counters = nullptr;
    events hooks;

    bool hello_sent = false;
    bool remote_hello_valid = false;
    microseconds opened_at{ 0 };
    microseconds last_rx{ 0 };
    microseconds last_tx{ 0 };
    microseconds ping_sent_at{ 0 };
    uint64_t ping_token = 0;
    bool ping_outstanding = false;
    microseconds current_now{ 0 };
    natural_t pump_counter = 0;

    void deliver(rx_event&& ev);
    void process_event(rx_event&& ev);
    void on_bytes(std::span<const uint8_t> bytes);
    void process_frame(parsed_frame&& frame);
    void handle_control(uint16_t net_id, std::span<const uint8_t> payload);
    void handle_hello(std::span<const uint8_t> payload);
    void try_advance_past_handshake();
    void apply_auth_result(link_security::auth_result result);
    void make_link_up();
    void send_hello();
    void send_control(mesh_message id, std::span<const uint8_t> payload);
    void protocol_error(std::string_view what);
    void run_timers(microseconds now);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_LINK_SINK_HPP
