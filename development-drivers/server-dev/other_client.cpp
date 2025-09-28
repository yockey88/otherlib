/**
 * \file server_dev/other_client.cpp
 **/
#include "other_client.hpp"

#include <algorithm>
#include <ranges>

#include "asio/asio/error.hpp"
#include "network_thread.hpp"

namespace other {

  void client::shutdown() {
    if (socket.is_open()) {
      socket.shutdown(asio::ip::tcp::socket::shutdown_both);
      socket.close();
    }
  }

  void client::start_read() {
    if (reading) {
      return;
    }
    reading = true;

    socket.async_receive(asio::buffer(read_buffer), std::bind_front(&client::finish_read, this));
  }

  void client::start_write(const std::vector<uint8_t>& data) {
    std::span buf{ write_buffer.begin(), write_buffer.size() };
    size_t offset = 0;
    do {
      std::span buffer = buf.subspan(offset);
      std::vector<uint8_t> chunk = {};
      if (buffer.size() < kBufferSize) {
        chunk = std::vector<uint8_t>(buffer.begin(), buffer.end());
      } else {
        chunk = std::vector<uint8_t>(buffer.begin(), buffer.begin() + kBufferSize);
      }
      write_queue.push(chunk);
      offset += kBufferSize;
    } while (offset < data.size());

    if (writing) {
      return;
    }
    writing = true;

    auto write_data = std::move(write_queue.front());
    write_queue.pop();

    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    socket.async_send(asio::buffer(write_buffer), std::bind_front(&client::finish_write, this));
  }

  void client::start_write() {
    if (writing) {
      return;
    }
    if (write_queue.empty()) {
      return;
    }

    writing = true;

    auto write_data = std::move(write_queue.front());
    write_queue.pop();

    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    socket.async_send(asio::buffer(write_buffer), std::bind_front(&client::finish_write, this));
  }

  void client::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    reading = false;
    if (ec && (ec == asio::error::operation_aborted || ec == asio::error::eof)) {
      // Connection closed cleanly by peer.
      thread->report_connection_closed(client_id);
      return;
    }

    if (!ec) {
      std::vector<uint8_t> data(read_buffer.begin(), read_buffer.begin() + bytes_transferred);
      read_queue.push(data);
    } else {
      thread->report_connection_error(this, ec);
      return;
    }

    socket.async_receive(asio::buffer(read_buffer), std::bind_front(&client::finish_read, this));
  }

  void client::finish_write(const asio::error_code& ec, std::size_t bytes_transferred) {
    writing = false;
    if (ec && ec == asio::error::operation_aborted) {
      // Connection closed cleanly by peer.
      return;
    }

    if (!ec) {
      if (!write_queue.empty()) {
        start_write();
      }
    } else {
      /// report error here
    }
  }

}  // namespace other