/**
 * \file network/session.cpp
 **/
#include "network/session.hpp"

#include <algorithm>

#include <asio/asio/error.hpp>

#include "network/network_thread.hpp"
#include "network/session_state_machine.hpp"

namespace other {

  session::session(session& other)
      : socket(std::move(other.socket)), io_context(other.io_context) {
    thread = other.thread;
    other.thread = nullptr;

    state_machine = other.state_machine;
    other.state_machine = {};

    reading = other.reading;
    read_queue = std::move(other.read_queue);
    read_buffer = std::move(other.read_buffer);

    writing = other.writing;
    write_queue = std::move(other.write_queue);
    write_buffer = std::move(other.write_buffer);
  }

  session& session::operator=(session&& other) {
    if (this != &other) {
      thread = other.thread;
      other.thread = nullptr;

      state_machine = other.state_machine;
      other.state_machine = {};

      reading = other.reading;
      read_queue = std::move(other.read_queue);
      read_buffer = std::move(other.read_buffer);

      writing = other.writing;
      write_queue = std::move(other.write_queue);
      write_buffer = std::move(other.write_buffer);
    }

    return *this;
  }

  void session::start_initialization() {
    CORE_LOG_DEBUG("Session {} starting initialization", session_id);
    state_machine.handle_event(network::SESSION_EVENT_START, this);
    start_read();
  }

  void session::checked_in() {
    CORE_LOG_DEBUG("Session {} checked in", session_id);
    state_machine.handle_event(network::SESSION_EVENT_CHECK_IN, this);
  }

  void session::check_in() {
    CORE_LOG_DEBUG("Session {} checking in", session_id);
    state_machine.handle_event(network::SESSION_EVENT_START, this);

    message msg;
    msg.header = {
      .category = CONTROL,
      .id = PING,
    };

    const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
    /// other session related data

    OTHER_ASSERT(msg.data.size() < kBufferSize, "Check-in message data is too large");
    dump_message_bytes(msg);
    start_write(std::move(msg));
  }

  void session::shutdown() {
    try {
      socket.close();
      state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_START, this);
    } catch (...) {
    }
  }

  void session::finalize() {
    CORE_LOG_DEBUG("Session {} finalized shutdown", session_id);
    state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
  }

  void session::start_read() {
    if (reading) {
      return;
    }
    reading = true;

    socket.async_receive(asio::buffer(read_buffer), std::bind_front(&session::finish_read, this));
  }

  void session::start_write(message&& msg) {
    size_t offset = 0;

    std::vector<uint8_t> msg_bytes = {};

    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);
    msg_bytes.append_range(std::span(header_bytes, sizeof(message_header)));
    msg_bytes.append_range(std::span(msg.data.data(), msg.data.size()));

    std::span buffer{ msg_bytes.data(), msg_bytes.size() };
    do {
      buffer = buffer.subspan(offset);

      std::vector<uint8_t> chunk = {};
      if (buffer.size() < kBufferSize) {
        chunk = { buffer.begin(), buffer.end() };
        offset += buffer.size();
      } else {
        chunk = { buffer.begin(), buffer.begin() + kBufferSize };
        offset += kBufferSize;
      }

      write_queue.push(std::move(chunk));
    } while (offset < msg_bytes.size());
    if (writing) {
      return;
    }

    writing = true;

    auto write_data = std::move(write_queue.front());
    write_queue.pop();

    std::ranges::fill(write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    std::span write_buffer_span(write_buffer.data(), write_data.size());
    dump_bytes_for_debug(write_buffer_span, std::format("writing {} bytes", write_data.size()));
    socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&session::finish_write, this));
  }

  void session::start_write() {
    if (writing) {
      return;
    }
    if (write_queue.empty()) {
      return;
    }

    writing = true;

    auto write_data = std::move(write_queue.front());
    write_queue.pop();

    std::ranges::fill(write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    std::span write_buffer_span(write_buffer.data(), write_data.size());
    dump_bytes_for_debug(write_buffer_span, std::format("writing {} bytes", write_data.size()));
    socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&session::finish_write, this));
  }

  void session::poll() {
    std::vector<uint8_t> data = {};
    do {
      data = try_receive();
      if (data.empty()) {
        continue;
      }
      OTHER_ASSERT(data.size() >= sizeof(message_header), "Invalid packet size!");

      std::span bytes{ data.data(), data.size() };
      dump_bytes_for_debug(bytes, std::format("received {} bytes", bytes.size()));

      message_header header = *reinterpret_cast<message_header*>(bytes.data());

      std::vector<uint8_t> payload = {};
      payload.append_range(bytes.subspan(sizeof(message_header)));

      switch (header.category) {
        case CONTROL:
          switch (header.id) {
            case PING: handle_control_ping(header, payload); break;
            case PONG: handle_control_pong(header, payload); break;
            default:
              CORE_LOG_WARN("Unknown CONTROL message ID {:#06x} from session {}", header.id, session_id);
              break;
          }
          break;

        default:
          CORE_LOG_WARN("Unknown message category {} from session {}", header.category, session_id);
          break;
      }

    } while (!data.empty());
  }

  std::vector<uint8_t> session::try_receive() {
    if (read_queue.empty()) {
      return {};
    }

    std::vector<uint8_t> data = std::move(read_queue.front());
    read_queue.pop();

    if (data.size() < sizeof(message_header)) {
      if (read_queue.empty()) {
        read_queue.push(std::move(data));
        return {};
      }

      std::vector<uint8_t> next_chunk = std::move(read_queue.front());
      read_queue.pop();
      data.append_range(next_chunk);

      if (data.size() < sizeof(message_header)) {
        read_queue.push(std::move(data));
        return {};
      }
    }

    return data;
  }

  void session::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    reading = false;
    if (state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN || state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      if (state_machine.get_current_state() != network::SESSION_STATE_STOPPED) {
        state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
        thread->report_connection_closed(session_id);
      }
      return;
    }

    if (ec) {
      CORE_LOG_TRACE("Read error on session {}: {}", session_id, ec.message());
    }

    if (ec && (ec == asio::error::operation_aborted || ec == asio::error::eof || ec == asio::error::connection_reset
#if 0
      || ec == asio::error::interrupted
#endif
              )) {
      // Connection closed cleanly by peer.
      thread->report_connection_closed(session_id);
      return;
    }

    if (!ec) {
      CORE_LOG_DEBUG("Session {} finished reading {} bytes", session_id, bytes_transferred);
      std::vector<uint8_t> data(read_buffer.begin(), read_buffer.begin() + bytes_transferred);
      dump_bytes_for_debug(data, "adding to read queue");
      read_queue.push(data);

      start_read();
    } else {
      thread->report_connection_error(this, ec);
      return;
    }

    socket.async_receive(asio::buffer(read_buffer), std::bind_front(&session::finish_read, this));
  }

  void session::finish_write(const asio::error_code& ec, std::size_t bytes_transferred) {
    writing = false;
    if (state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN || state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      if (state_machine.get_current_state() != network::SESSION_STATE_STOPPED) {
        state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
        thread->report_connection_closed(session_id);
      }
      return;
    }

    if (ec && (ec == asio::error::operation_aborted || ec == asio::error::eof || ec == asio::error::connection_reset
#if 0
      || ec == asio::error::interrupted
#endif
              )) {
      // Connection closed cleanly by peer.
      thread->report_connection_closed(session_id);
      return;
    }

    if (!ec) {
      /// \todo successful write callback ?
      start_write();
    } else {
      /// report error here
    }
  }

  void session::handle_control_ping(const message_header& header, const std::span<uint8_t> data) {
    CORE_LOG_DEBUG("Received PING from session {}", session_id);
    switch (state_machine.get_current_state()) {
      case network::SESSION_STATE_LAUNCHING: {
        CORE_LOG_DEBUG("Received PING from session {}", session_id);

        integer_t received_session_id = 0;
        if (data.size() < sizeof(integer_t)) {
          throw network_packet_parse_error("Invalid PING message data size");
        }

        received_session_id = *reinterpret_cast<const integer_t*>(data.data());
        if (received_session_id != session_id) {
          CORE_LOG_WARN("Received PING with mismatched session ID {} (expected {})", received_session_id, session_id);
          return;
        }

        // Respond with PONG
        message pong_msg;
        pong_msg.header = {
          .category = CONTROL,
          .id = PONG,
        };

        start_write(std::move(pong_msg));

        // replace server's connection id with new session id
        thread->report_connection_check_in(session_id, received_session_id);
        session_id = received_session_id;

        state_machine.handle_event(network::SESSION_EVENT_CHECK_IN, this);
      } break;
      default:
        CORE_LOG_WARN("Received PING in unexpected state {} for session {}", static_cast<int>(state_machine.get_current_state()), session_id);
        break;
    }
  }

  void session::handle_control_pong(const message_header& header, const std::span<uint8_t> data) {
    CORE_LOG_DEBUG("Received PONG from session {}", session_id);

    switch (state_machine.get_current_state()) {
      case network::SESSION_STATE_LAUNCHING: checked_in(); break;
      default:
        CORE_LOG_WARN("Received PONG in unexpected state {} for session {}", static_cast<int>(state_machine.get_current_state()), session_id);
        break;
    }
  }

  namespace {

    void write_bytes(std::ostream& os, const std::span<const uint8_t> data) {
      for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) {
          os << "\n";
        }
        os << std::format("{:#02x} ", data[i]);
      }
    }

  }  // namespace

  void session::dump_bytes_for_debug(const std::span<uint8_t> data, const std::string_view msg) {
    std::stringstream ss;
    ss << std::format("\n[session {}]:", session_id);
    if (!msg.empty()) {
      ss << "\n[" << msg << "]";
    }
    write_bytes(ss, data);
    CORE_LOG_TRACE("{}", ss.str());
  }

  void session::dump_message_bytes(const message& msg) {
    std::stringstream ss;
    ss << std::format("\nMESSAGE {}:", msg.header);
    write_bytes(ss, msg.data);
    CORE_LOG_TRACE("{}", ss.str());
  }

}  // namespace other