/**
 * \file network/tcp/connection.cpp
 **/
#include "network/tcp/connection.hpp"

#include "core/profiler.hpp"

#include "network/tcp/tcp_transport_provider.hpp"

#include "asio/asio/error.hpp"

namespace other {

  namespace {

    inline bool is_disconnect(const asio::error_code& ec) {
      return ec == asio::error::operation_aborted ||
        ec == asio::error::connection_reset ||
        ec == asio::error::timed_out ||
        ec == asio::error::eof;
    }

  }  // namespace

  bool connection::send(ostd::vector<uint8_t>&& bytes) {
    PROFILE_SECTION("connection::send");
    if (closing || bytes.empty()) {
      return false;
    }

    if (queued_bytes + bytes.size() > kMaxQueuedBytes) {
      CORE_LOG_WARN("[CONNECTION {}] send queue overflow ({} + {} bytes) — closing", id, queued_bytes, bytes.size());
      close(connection_close_reason::BACKPRESSURE);
      return false;
    }

    queued_bytes += bytes.size();
    send_queue.push_back(std::move(bytes));
    kick_write();
    return true;
  }

  void connection::kick_write() {
    if (write_in_flight || closing || send_queue.empty()) {
      return;
    }

    write_in_flight = true;
    /// the composed operation owns partial-write handling; the front element is stable
    ///  until completion pops it
    asio::async_write(tcp_socket, asio::buffer(send_queue.front().data(), send_queue.front().size()),
                      std::bind_front(&connection::on_write_complete, this));
  }

  void connection::on_write_complete(const asio::error_code& ec, size_t bytes_transferred) {
    PROFILE_SECTION("connection::on_write_complete");
    write_in_flight = false;
    if (closing) {
      return;
    }

    if (ec) {
      CORE_LOG_TRACE("[CONNECTION {}: WRITE FAILED] {}", id, ec.message());
      close(is_disconnect(ec) ? connection_close_reason::REMOTE_CLOSED : connection_close_reason::WRITE_ERROR);
      return;
    }

    OTHER_ASSERT(!send_queue.empty(), "Write completed with an empty send queue on connection {}", id);
    queued_bytes -= send_queue.front().size();
    send_queue.pop_front();
    kick_write();
  }

  void connection::close(connection_close_reason reason) {
    PROFILE_SECTION("connection::close");
    if (closing) {
      return;
    }
    closing = true;
    close_reason = reason;
    /// the in-flight front buffer must outlive its (now aborting) async_write — the
    ///  kernel may still be reading it until the completion fires
    if (write_in_flight && !send_queue.empty()) {
      send_queue.erase(send_queue.begin() + 1, send_queue.end());
    } else {
      send_queue.clear();
    }
    queued_bytes = 0;

    asio::error_code ec;
    tcp_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    tcp_socket.close(ec);

    provider->notify_conn_closed(id, reason);
  }

  void connection::start_connect(const asio::ip::tcp::endpoint& remote) {
    PROFILE_SECTION("connection::start_connect");
    OTHER_ASSERT(!connect_in_flight, "Connection {} is already connecting", id);
    connect_in_flight = true;
    tcp_socket.async_connect(remote, std::bind_front(&connection::on_connect_complete, this));
  }

  void connection::on_connect_complete(const asio::error_code& ec) {
    PROFILE_SECTION("connection::on_connect_complete");
    connect_in_flight = false;
    if (closing) {
      return;
    }

    if (ec) {
      /// dial failure is data, not an error condition of ours
      CORE_LOG_TRACE("[CONNECTION {}: CONNECT FAILED] {}", id, ec.message());
      closing = true;
      close_reason = connection_close_reason::CONNECT_FAILED;
      asio::error_code ignored;
      tcp_socket.close(ignored);
      provider->notify_connect_failed(id);
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: CONNECT SUCCESS]", id);
    provider->notify_connect_succeeded(id);
  }

  void connection::begin_read() {
    PROFILE_SECTION("connection::begin_read");
    if (read_in_flight || closing) {
      return;
    }

    read_in_flight = true;
    tcp_socket.async_read_some(asio::buffer(read_chunk), std::bind_front(&connection::on_read_complete, this));
  }

  void connection::on_read_complete(const asio::error_code& ec, size_t bytes_transferred) {
    PROFILE_SECTION("connection::on_read_complete");
    read_in_flight = false;
    if (closing) {
      return;
    }

    if (ec) {
      CORE_LOG_TRACE("[CONNECTION {}: READ CLOSED] {}", id, ec.message());
      close(is_disconnect(ec) ? connection_close_reason::REMOTE_CLOSED : connection_close_reason::READ_ERROR);
      return;
    }

    if (bytes_transferred > 0) {
      provider->notify_conn_rx(id, std::span<const uint8_t>(read_chunk.data(), bytes_transferred));
    }
    begin_read();
  }

  asio::ip::tcp::endpoint connection::get_remote_endpoint() const {
    asio::error_code ec;
    return tcp_socket.remote_endpoint(ec);
  }

  asio::ip::tcp::endpoint connection::get_local_endpoint() const {
    asio::error_code ec;
    return tcp_socket.local_endpoint(ec);
  }

}  // namespace other