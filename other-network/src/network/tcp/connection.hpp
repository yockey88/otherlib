/**
 * \file network/tcp/connection.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TCP_CONNECTION_HPP
#define OTHER_NETWORK_NETWORK_TCP_CONNECTION_HPP

#include <array>
#include <deque>

#include "network/transport_provider.hpp"

#include "message/message.hpp"

namespace other {

  class tcp_transport_provider;

  /// one tcp socket, network-thread-only. tx is a send queue drained by the composed
  ///  asio::async_write (partial writes are its contract, not ours); rx is chunk reads
  ///  fanned out as raw bytes — framing is the mesh's business, not the transport's
  class connection {
   public:
    constexpr static size_t kReadChunkSize = 8192;             // read granularity, NOT a payload cap
    constexpr static size_t kMaxQueuedBytes = 4 * 1024 * 1024;  // beyond: close(BACKPRESSURE)

    connection(tcp_transport_provider* provider, natural_t id, asio::ip::tcp::socket tcp_socket)
        : provider(provider), id(id), tcp_socket(std::move(tcp_socket)) {}
    ~connection() = default;

    /// queue; never blocks. false = closing or refused (overflow closes the connection)
    bool send(ostd::vector<uint8_t>&& bytes);
    /// idempotent; the reason of the FIRST close wins and rides the closed notification
    void close(connection_close_reason reason);

    void start_connect(const asio::ip::tcp::endpoint& remote);
    void begin_read();

    bool is_closing() const { return closing; }
    /// an async handler still holds this object; destruction must wait
    bool busy() const { return read_in_flight || write_in_flight || connect_in_flight; }
    size_t queued() const { return queued_bytes; }

    asio::ip::tcp::endpoint get_remote_endpoint() const;
    asio::ip::tcp::endpoint get_local_endpoint() const;

   private:
    tcp_transport_provider* provider = nullptr;
    natural_t id = 0;
    asio::ip::tcp::socket tcp_socket;

    std::deque<ostd::vector<uint8_t>> send_queue;
    size_t queued_bytes = 0;
    bool write_in_flight = false;
    bool read_in_flight = false;
    bool connect_in_flight = false;

    bool closing = false;
    connection_close_reason close_reason = connection_close_reason::NONE;

    std::array<uint8_t, kReadChunkSize> read_chunk{};

    void kick_write();
    void on_connect_complete(const asio::error_code& ec);
    void on_read_complete(const asio::error_code& ec, size_t bytes_transferred);
    void on_write_complete(const asio::error_code& ec, size_t bytes_transferred);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TCP_CONNECTION_HPP