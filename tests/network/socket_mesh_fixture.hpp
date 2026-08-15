/**
 * \file network/socket_mesh_fixture.hpp
 **/
#ifndef OTHER_TESTS_NETWORK_SOCKET_MESH_FIXTURE_HPP
#define OTHER_TESTS_NETWORK_SOCKET_MESH_FIXTURE_HPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "core/scope.hpp"
#include "core/time.hpp"

#include "network/network_thread.hpp"
#include "network/packet_sink.hpp"
#include "network/tcp/tcp_transport_provider.hpp"
#include "network/udp/udp_transport_provider.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "message/message_bus.hpp"

namespace other {

  /// transport-wide or conn-scoped byte recorder; events land synchronously on the
  ///  network thread, read from the test thread under the mutex
  class recording_sink final : public packet_sink {
   public:
    void rx_data(natural_t conn_id, std::span<const uint8_t> data) override {
      std::lock_guard lock(mutex);
      auto& stream = streams[conn_id];
      stream.insert(stream.end(), data.begin(), data.end());
      chunk_counts[conn_id]++;
      total_bytes += data.size();
    }
    void connection_opened(natural_t conn_id) override {
      std::lock_guard lock(mutex);
      opened.push_back(conn_id);
    }
    void connection_closed(natural_t conn_id) override {
      std::lock_guard lock(mutex);
      closed.push_back(conn_id);
    }

    std::vector<uint8_t> stream_of(natural_t conn_id) {
      std::lock_guard lock(mutex);
      return streams[conn_id];
    }
    size_t chunks_of(natural_t conn_id) {
      std::lock_guard lock(mutex);
      return chunk_counts[conn_id];
    }
    size_t total() {
      std::lock_guard lock(mutex);
      return total_bytes;
    }
    std::vector<natural_t> opened_ids() {
      std::lock_guard lock(mutex);
      return opened;
    }
    std::vector<natural_t> closed_ids() {
      std::lock_guard lock(mutex);
      return closed;
    }

   private:
    std::mutex mutex;
    std::map<natural_t, std::vector<uint8_t>> streams;
    std::map<natural_t, size_t> chunk_counts;
    size_t total_bytes = 0;
    std::vector<natural_t> opened;
    std::vector<natural_t> closed;
  };

  /// one process-half of a socket conversation: bus + network thread + tcp/udp providers +
  ///  one mesh attached to both; two localhost instances form the conformance harness
  struct socket_net_instance {
    message_bus bus;
    network_thread thread{ bus };
    scope<tcp_transport_provider> tcp = make_scope<tcp_transport_provider>();
    scope<udp_transport_provider> udp = make_scope<udp_transport_provider>();
    peer_mesh mesh;

    /// every lifecycle notification the driver side saw, raw commands included
    std::vector<notification_connection_opened> opened_notes;
    std::vector<notification_connection_closed> closed_notes;
    std::vector<acknowledgement_ack> acks;

    std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::now();
    natural_t next_ack_id = 1;
    bool started = false;
    bool stopped = false;

    explicit socket_net_instance(std::string_view name, const peer_mesh_config& cfg = fast_socket_cfg())
        : mesh(name, cfg) {
      mesh.attach_provider(*tcp);
      mesh.attach_provider(*udp);
    }

    ~socket_net_instance() { stop(); }

    /// sockets ride real time; timers sized so idle paths run in test-scale millis
    static peer_mesh_config fast_socket_cfg() {
      peer_mesh_config cfg;
      cfg.handshake_timeout = microseconds{ 2'000'000 };
      cfg.keepalive_idle = microseconds{ 150'000 };
      cfg.link_timeout = microseconds{ 600'000 };
      return cfg;
    }

    bool start() {
      thread.launch();
      bus.register_thread();

      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
      bool ready = false;
      while (!ready && std::chrono::steady_clock::now() < deadline) {
        opt<message> msg = bus.try_receive_message();
        if (!msg.has_value()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }
        ready = message_header{ msg->category, msg->id } == message_header{ NOTIFICATION, NETWORK_THREAD_READY };
      }
      if (!ready) {
        return false;
      }

      thread.register_provider(tcp.get());
      thread.register_provider(udp.get());
      started = true;
      return true;
    }

    microseconds now() const {
      return duration_cast<microseconds>(std::chrono::steady_clock::now() - epoch);
    }

    /// drain notifications into the note log (raw-row asserts), tick the mesh — link
    ///  establishment and rx flow through the providers' own sinks
    void pump() {
      while (opt<message> msg = bus.try_receive_message()) {
        route_message(*msg);
      }
      mesh.tick(now());
    }

    void route_message(const message& msg) {
      if (msg.category == NOTIFICATION && msg.id == CONNECTION_OPENED) {
        opened_notes.push_back(deserialize_direct<notification_connection_opened>(msg.data).first);
        return;
      }
      if (msg.category == NOTIFICATION && msg.id == CONNECTION_CLOSED) {
        closed_notes.push_back(deserialize_direct<notification_connection_closed>(msg.data).first);
        return;
      }
      if (msg.category == ACKNOWLEDGEMENT && msg.id == ACK) {
        acks.push_back(deserialize_direct<acknowledgement_ack>(msg.data).first);
        return;
      }
    }

    /// ------------------------------------------------------------ raw provider ops
    /// byte-row helpers: drive the provider through the bus with no mesh involved

    void send_raw_command(uint16_t id, ostd::vector<uint8_t>&& data) {
      message msg(COMMAND, id);
      msg.data = std::move(data);
      bus.send_message(std::move(msg));
    }

    /// ack-wrapped so the caller can wait on success/failure deterministically
    natural_t send_acked_command(uint16_t id, ostd::vector<uint8_t>&& data) {
      const natural_t ack_id = next_ack_id++;
      message inner(COMMAND, id);
      message wrapper(REQUEST, ACK);
      request_acknowledgment request{
        .ack_id = ack_id,
        .original_header = { inner.category, inner.id },
        .message_data = std::move(data),
      };
      wrapper.data = serialize_direct(request);
      bus.send_message(std::move(wrapper));
      return ack_id;
    }

    opt<uint8_t> ack_result(natural_t ack_id) {
      for (const acknowledgement_ack& ack : acks) {
        if (ack.ack_id == ack_id) {
          return ack.ack;
        }
      }
      return std::nullopt;
    }

    struct raw_listen_result {
      natural_t listener_id = 0;
      natural_t ack_id = 0;
    };

    raw_listen_result raw_listen(uint16_t port, std::string_view transport = "tcp") {
      const natural_t listener_id = thread.generate_connection_id();
      command_listen_connection request{
        .endpoint = { network_system_localhost(), port },
        .connection_id = listener_id,
        .transport_hash = transport_provider::hash_name(transport),
      };
      const natural_t ack_id = send_acked_command(LISTEN_CONNECTION, serialize_direct(request));
      return { listener_id, ack_id };
    }

    natural_t raw_connect(uint16_t port, std::string_view transport = "tcp") {
      const natural_t conn_id = thread.generate_connection_id();
      command_connect_connection request{
        .endpoint = { network_system_localhost(), port },
        .connection_id = conn_id,
        .transport_hash = transport_provider::hash_name(transport),
      };
      send_raw_command(CONNECT_CONNECTION, serialize_direct(request));
      return conn_id;
    }

    void raw_tx(natural_t conn_id, std::span<const uint8_t> data) {
      command_tx_data request{
        .connection_id = conn_id,
        .data = ostd::vector<uint8_t>(data.begin(), data.end()),
      };
      send_raw_command(TX_DATA, serialize_direct(request));
    }

    void raw_close(natural_t conn_id) {
      command_close_connection request{ .connection_id = conn_id, .transport_hash = 0 };
      send_raw_command(CLOSE_CONNECTION, serialize_direct(request));
    }

    static constexpr uint32_t network_system_localhost() { return 0x7f000001; }

    bool has_opened_note(natural_t conn_id) const {
      return std::ranges::find(opened_notes, conn_id, &notification_connection_opened::connection_id) != opened_notes.end();
    }
    const notification_connection_opened* opened_note(natural_t conn_id) const {
      auto itr = std::ranges::find(opened_notes, conn_id, &notification_connection_opened::connection_id);
      return itr != opened_notes.end() ? &*itr : nullptr;
    }
    const notification_connection_opened* accepted_note_for_listener(natural_t listener_id) const {
      auto itr = std::ranges::find(opened_notes, listener_id, &notification_connection_opened::listener_id);
      return itr != opened_notes.end() ? &*itr : nullptr;
    }
    const notification_connection_closed* closed_note(natural_t conn_id) const {
      auto itr = std::ranges::find(closed_notes, conn_id, &notification_connection_closed::connection_id);
      return itr != closed_notes.end() ? &*itr : nullptr;
    }

    void stop() {
      if (!started || stopped) {
        started = false;
        return;
      }
      stopped = true;

      /// runner-pattern shutdown: ack-wrapped request, deferred ack arrives once every
      ///  route has drained
      const natural_t ack_id = send_acked_command(SHUTDOWN_REQUEST, {});
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
      while (!ack_result(ack_id).has_value() && std::chrono::steady_clock::now() < deadline) {
        while (opt<message> msg = bus.try_receive_message()) {
          route_message(*msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      thread.shutdown();
      thread.wait_for_shutdown_complete();
    }
  };

  /// pump a set of instances until the predicate holds or the deadline lapses
  template <typename Pred>
  bool pump_until(std::initializer_list<socket_net_instance*> instances, Pred&& done,
                  std::chrono::milliseconds deadline = std::chrono::milliseconds(3000)) {
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < deadline) {
      for (socket_net_instance* instance : instances) {
        instance->pump();
      }
      if (done()) {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return done();
  }

  /// unique-ish localhost ports per test run; shuffled suites must not collide
  inline uint16_t next_test_port() {
    static std::atomic<uint16_t> next{ 43500 };
    return next.fetch_add(1, std::memory_order_relaxed);
  }

}  // namespace other

#endif  // OTHER_TESTS_NETWORK_SOCKET_MESH_FIXTURE_HPP