/**
 * \file network/udp_stream.cpp
 **/
#include "network/udp_stream.hpp"

#include <cstdint>

#include "thread/message.hpp"

#include "network/network_thread.hpp"

#include "asio/asio/error.hpp"
#include "message.hpp"

namespace other {

  udp_stream::udp_stream(network_thread* thread, natural_t connection_id, integer_t stream_id, asio::io_context& io_context, const asio::ip::udp::endpoint& endpoint, const asio::ip::udp::endpoint& remote_endpoint)
      : socket(io_context, asio::ip::udp::v4()), endpoint(endpoint), remote_endpoint(remote_endpoint), connection_id(connection_id), stream_id(stream_id), io_context(io_context), thread(thread) {
    this_handle.stream = this;
    this_handle.stream_mutex = &stream_mutex;

    try {
      socket.set_option(asio::ip::udp::socket::reuse_address(true));
      socket.bind(endpoint);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error opening UDP socket for stream {} on connection {}: {}", stream_id, connection_id, e.what());
    }
  }

  void udp_stream::poll() {
    auto full_data = try_receive();
    if (!full_data.empty()) {
      CORE_LOG_DEBUG("UDP Full Message Received. Pushing full data to queue ({} bytes)", full_data.size());
      full_data_queue.push(std::move(full_data));
    }

    if (!full_data_queue.empty()) {
      CORE_LOG_DEBUG("UDP Full Data Queue has {} messages pending", full_data_queue.size());

      auto data = std::move(full_data_queue.front());
      full_data_queue.pop();

      auto bytes = std::span(data);

      message msg;
      msg.header = {
        .category = NOTIFICATION,
        .id = STREAM_RX_UDP_DATAGRAM,
      };

      notification_stream_rx_datagram notif;
      notif.stream_id = stream_id;
      notif.datagram.type = static_cast<udp_packet_type>(bytes[0]);
      bytes = bytes.subspan(sizeof(uint8_t));
      switch (notif.datagram.type) {
        case udp_packet_type::UDP_CHECK_IN:
          OTHER_ASSERT(bytes.size() >= sizeof(udp_check_in), "Invalid UDP check-in packet size");
          notif.datagram.packet.check_in = *reinterpret_cast<const udp_check_in*>(bytes.data());
          bytes = bytes.subspan(sizeof(udp_check_in));
          break;
        default:
          OTHER_ASSERT(false, "Unknown UDP packet type in notification_stream_rx_datagram");
      }

      msg.data.append_range(notif.as_buffer());
      thread->get_message_bus().send_message(std::move(msg));
    }
  }

  void udp_stream::start_read() {
    if (buffer.reading) {
      return;
    }
    buffer.reading = true;
    socket.async_receive_from(asio::buffer(buffer.read_buffer), remote_endpoint, std::bind_front(&udp_stream::finish_read, this));
  }

  void udp_stream::shutdown() {
    CORE_LOG_DEBUG("Shutting down UDP stream {} on connection {}", stream_id, connection_id);
    try {
      socket.close();
    } catch (...) {
    }

    if (!buffer.reading && !buffer.writing) {
      CORE_LOG_DEBUG("  > UDP stream inactive, shutting down immediately");
      thread->report_stream_closed(connection_id, stream_id);
    }
  }

  std::vector<uint8_t> udp_stream::receive() {
    start_read();

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

    uint16_t size = static_cast<uint16_t>(msg.data.size());
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&size);
    data.append_range(std::span(header_bytes, sizeof(message_header)));
    data.append_range(std::span(size_bytes, sizeof(uint16_t)));
    data.append_range(msg.data);

    dump_bytes_for_debug(data, std::format("[TX MESSAGE] {}", msg.header));
    send(data);
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
    dump_bytes_for_debug(write_data, std::format("[WRITING CHUNK] {} bytes", write_data.size()));

    std::ranges::fill(buffer.write_buffer, 0);
    std::ranges::copy(write_data, buffer.write_buffer.begin());

    socket.async_send_to(asio::buffer(buffer.write_buffer.data(), write_data.size()), remote_endpoint, std::bind_front(&udp_stream::finish_write, this));
    CORE_LOG_TRACE("UDP stream writing started ({} bytes)", write_data.size());
  }

  void udp_stream::start_write(const std::span<const uint8_t> data) {
    std::vector<uint8_t> msg_bytes = {};

    size_t offset = 0;
    do {
      auto bytes = data.subspan(offset);

      std::vector<uint8_t> chunk = {};
      if (data.size() < kBufferSize) {
        chunk.append_range(bytes);
        offset += data.size();
      } else {
        chunk.append_range(bytes.subspan(0, kBufferSize));
        offset += kBufferSize;
      }

      dump_bytes_for_debug(chunk, std::format("[TX CHUNK] {} bytes", chunk.size()));
      buffer.write_queue.push_back(std::move(chunk));
    } while (offset < msg_bytes.size());
    if (buffer.writing) {
      return;
    }

    start_write();
  }

  void udp_stream::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    buffer.reading = false;
    if (ec && (ec == asio::error::operation_aborted || ec == asio::error::connection_reset || ec == asio::error::connection_aborted || ec == asio::error::eof)) {
      CORE_LOG_DEBUG("UDP Read Error: Connection closed");
      thread->report_stream_closed(connection_id, stream_id);
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("UDP READ ERROR");
      thread->report_stream_error(this, ec);
      return;
    }

    std::vector<uint8_t> data(buffer.read_buffer.begin(), buffer.read_buffer.begin() + bytes_transferred);
    dump_bytes_for_debug(data, "[RX CHUNK]");
    buffer.read_queue.push_back(data);

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
      start_write();
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

  namespace {

    void write_bytes(std::ostream& os, const std::span<const uint8_t> data) {
      for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) {
          os << "\n";
        }
        os << std::format("{:#04x} ", data[i]);
      }
    }

  }  // namespace

  void udp_stream::dump_bytes_for_debug(const std::span<uint8_t> data, const std::string_view msg) {
    std::stringstream ss;
    ss << std::format("\n[udp stream {}]:", connection_id);
    if (!msg.empty()) {
      ss << "\n[" << msg << "]";
    }
    write_bytes(ss, data);
    CORE_LOG_TRACE("{}", ss.str());
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