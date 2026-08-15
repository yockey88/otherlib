/**
 * \file network/udp/udp_transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_UDP_UDP_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_UDP_UDP_TRANSPORT_PROVIDER_HPP

#include <array>
#include <deque>

#include "network/transport_provider.hpp"

namespace other {

  /// the real datagram transport: unreliable, unordered, 1 datagram = 1 delivery, uninterpreted.
  ///  connectionless, so connections are synthesized; liveness lives above (mesh LINK_HELLO/keepalive) — no handshake, no EOF
  class udp_transport_provider : public socket_transport_provider {
   public:
    /// safe-MTU posture: radio/MANET conditions sit well below ethernet's 1472, and ip
    ///  fragmentation is the failure mode a datagram transport must avoid (ctor-set, provider-internal)
    constexpr static uint32_t kDefaultMaxDatagramBytes = 1200;

    explicit udp_transport_provider(uint32_t max_datagram_bytes = kDefaultMaxDatagramBytes)
        : max_datagram_bytes(max_datagram_bytes) {}
    virtual ~udp_transport_provider() = default;

    bool is_stream() const override { return false; }
    link_caps conn_caps(natural_t) const override {
      return { .reliable = false, .ordered = false, .max_frame_size = max_datagram_bytes };
    }

    std::string name() const override { return "UDP"; }

    bool is_reliable() const override { return false; }
    bool is_ordered() const override { return false; }
    bool is_datagram() const override { return true; }

    uint32_t max_datagram() const { return max_datagram_bytes; }
    /// counted on the network thread, readable anywhere
    natural_t oversize_tx_refusals() const { return oversize_refused.load(std::memory_order_relaxed); }

   private:
    constexpr static size_t kRecvBufferSize = 64 * 1024;
    constexpr static size_t kMaxQueuedDatagrams = 256;

    /// one bound socket: a listener (synthesizes conns per remote) or a dial (one conn)
    struct socket_entry {
      scope<asio::ip::udp::socket> socket;
      bool listener = false;
      /// dial sockets: the one expected remote and its conn id
      asio::ip::udp::endpoint dial_remote{};
      natural_t dial_conn = 0;
      std::map<asio::ip::udp::endpoint, natural_t> remotes;

      std::array<uint8_t, kRecvBufferSize> recv_buffer{};
      asio::ip::udp::endpoint recv_from{};
      bool recv_in_flight = false;

      std::deque<std::pair<asio::ip::udp::endpoint, ostd::vector<uint8_t>>> send_queue;
      bool send_in_flight = false;
      bool closing = false;
    };

    struct conn_info {
      natural_t socket_id = 0;
      asio::ip::udp::endpoint remote{};
    };

    uint32_t max_datagram_bytes = kDefaultMaxDatagramBytes;
    std::atomic<natural_t> oversize_refused = 0;

    std::map<natural_t, scope<socket_entry>> sockets;
    std::map<natural_t, conn_info> conns;
    std::deque<natural_t> pending_destroy;

    void on_initialize() override;
    void on_tick() override;
    void on_begin_shutdown() override;
    void on_shutdown() override;

    void on_start_listen(natural_t conn_id, const binding_point& endpoint) override;
    void on_start_connect(natural_t conn_id, const binding_point& endpoint) override;
    void tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) override;
    void net_close(natural_t connection_id) override;
    void connection_removed(natural_t connection_id) override;

    void begin_receive(natural_t socket_id);
    void on_received(natural_t socket_id, const asio::error_code& ec, size_t bytes);
    void kick_send(natural_t socket_id);
    void on_sent(natural_t socket_id, const asio::error_code& ec, size_t bytes);

    void teardown_socket(natural_t socket_id, connection_close_reason reason);
    void destroy_if_idle(natural_t socket_id);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_UDP_UDP_TRANSPORT_PROVIDER_HPP