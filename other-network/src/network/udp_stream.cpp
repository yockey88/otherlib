/**
 * \file network/udp_stream.cpp
 **/
#include "network/udp_stream.hpp"

#include <cstdint>

#include "thread/message.hpp"

namespace other {

  void udp_stream::start_read() {
    if (buffer.reading) {
      return;
    }
    buffer.reading = true;

    socket.async_receive(asio::buffer(buffer.read_buffer), std::bind_front(&udp_stream::finish_read, this));
  }

  void udp_stream::shutdown() {
    socket.close();
  }

  std::vector<uint8_t> udp_stream::receive() {
    if (full_data_queue.empty()) {
      return {};
    }
    std::vector<uint8_t> data = std::move(full_data_queue.front());
    full_data_queue.pop();
    return data;
  }

  void udp_stream::send(const std::span<const uint8_t> data) {
    start_write(data);
  }

  void udp_stream::send(message&& msg) {
    std::vector<uint8_t> data = {};

    auto bytes = std::span(msg.data);

    uint16_t size = static_cast<uint16_t>(bytes.size());
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&size);
    data.append_range(std::span(header_bytes, sizeof(message_header)));
    data.append_range(std::span(size_bytes, sizeof(uint16_t)));
    data.append_range(bytes);
    start_write(data);
  }

  void udp_stream::start_write() {
    if (buffer.writing) {
      return;
    }
    if (buffer.write_queue.empty()) {
      buffer.writing = false;
      return;
    }

    buffer.writing = true;
    auto write_data = std::move(buffer.write_queue.front());
    buffer.write_queue.pop_front();

    std::ranges::fill(buffer.write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), buffer.write_buffer.begin());
    socket.async_send(asio::buffer(buffer.write_buffer.data(), write_data.size()), std::bind_front(&udp_stream::finish_write, this));
  }

  void udp_stream::start_write(const std::span<const uint8_t> data) {
    std::vector<uint8_t> msg_bytes = {};

    size_t offset = 0;
    std::span tx_buffer{ msg_bytes.data(), msg_bytes.size() };
    do {
      tx_buffer = tx_buffer.subspan(offset);

      std::vector<uint8_t> chunk = {};
      if (tx_buffer.size() < kBufferSize) {
        chunk = { tx_buffer.begin(), tx_buffer.end() };
        offset += tx_buffer.size();
      } else {
        chunk = { tx_buffer.begin(), tx_buffer.begin() + kBufferSize };
        offset += kBufferSize;
      }

      buffer.write_queue.push_back(std::move(chunk));
    } while (offset < msg_bytes.size());
    if (buffer.writing) {
      return;
    }

    start_write();
  }

  void udp_stream::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    buffer.reading = false;
    if (ec) {
      // Handle error
      return;
    }

    CORE_LOG_TRACE("UDP stream finished reading {} bytes", bytes_transferred);
    std::vector<uint8_t> data(buffer.read_buffer.begin(), buffer.read_buffer.begin() + bytes_transferred);
    buffer.read_queue.push_back(data);

    auto full_data = try_receive();
    if (!full_data.empty()) {
      full_data_queue.push(std::move(full_data));
    }

    start_read();
  }

  void udp_stream::finish_write(const asio::error_code& ec, std::size_t bytes_transferred) {
    buffer.writing = false;
    if (ec) {
      // Handle error
      return;
    }

    CORE_LOG_TRACE("UDP stream finished writing {} bytes", bytes_transferred);

    if (!buffer.write_queue.empty()) {
      auto write_data = std::move(buffer.write_queue.front());
      buffer.write_queue.pop_front();

      std::ranges::fill(buffer.write_buffer, 0);
      std::ranges::copy(write_data.begin(), write_data.end(), buffer.write_buffer.begin());
      std::span write_buffer_span(buffer.write_buffer.data(), write_data.size());
      socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&udp_stream::finish_write, this));
    }
  }

  std::vector<uint8_t> udp_stream::try_receive() {
    if (buffer.read_queue.empty()) {
      return {};
    }

    std::vector<uint8_t> data = std::move(buffer.read_queue.front());
    buffer.read_queue.pop_front();

    if (data.size() < sizeof(message_header) + sizeof(uint16_t)) {
      if (buffer.read_queue.empty()) {
        buffer.read_queue.push_front(std::move(data));
        return {};
      }

      do {
        std::vector<uint8_t> next_chunk = std::move(buffer.read_queue.front());
        buffer.read_queue.pop_front();
        data.append_range(next_chunk);
      } while (data.size() < sizeof(message_header) + sizeof(uint16_t) && !buffer.read_queue.empty());
      if (data.size() < sizeof(message_header) + sizeof(uint16_t)) {
        buffer.read_queue.push_front(std::move(data));
        return {};
      }

      uint16_t message_size = *reinterpret_cast<const uint16_t*>(std::span(data).subspan(sizeof(message_header)).data());
      while (data.size() < sizeof(message_header) + sizeof(uint16_t) + message_size && !buffer.read_queue.empty()) {
        std::vector<uint8_t> next_chunk = std::move(buffer.read_queue.front());
        buffer.read_queue.pop_front();
        data.append_range(next_chunk);
      }
      if (data.size() < sizeof(message_header) + sizeof(uint16_t) + message_size) {
        buffer.read_queue.push_front(std::move(data));
        return {};
      }
    }

    return data;
  }

  void udp_handle::send(const std::span<const uint8_t> data) {
    OTHER_ASSERT(stream != nullptr, "UDP handle has no stream assigned");
    OTHER_ASSERT(stream_mutex != nullptr, "UDP handle has no mutex assigned");
    std::lock_guard lock(*stream_mutex);
    stream->send(data);
  }

  void udp_handle::send(message&& msg) {
    OTHER_ASSERT(stream != nullptr, "UDP handle has no stream assigned");
    OTHER_ASSERT(stream_mutex != nullptr, "UDP handle has no mutex assigned");
    std::lock_guard lock(*stream_mutex);
    stream->send(std::move(msg));
  }

  std::vector<uint8_t> udp_handle::receive() {
    OTHER_ASSERT(stream != nullptr, "UDP handle has no stream assigned");
    OTHER_ASSERT(stream_mutex != nullptr, "UDP handle has no mutex assigned");
    std::lock_guard lock(*stream_mutex);
    return stream->receive();
  }

}  // namespace other