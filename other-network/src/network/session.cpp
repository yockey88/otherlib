/**
 * \file network/session.cpp
 **/
#include "network/session.hpp"

#include <algorithm>
#include <cstdint>

#include <asio/asio/error.hpp>

#include "thread/message.hpp"

#include "network/network_thread.hpp"
#include "network/protocols/check_in_protocol.hpp"
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

    active_protocol_handler = make_scope<server_check_in_handler>(this);
    CORE_LOG_DEBUG("Launching [{}] protocol handler for session {}", active_protocol_handler->get_protocol_id(), session_id);

    start_read();
  }

  void session::checked_in() {
    state_machine.handle_event(network::SESSION_EVENT_CHECK_IN, this);

    // replace server's connection id with new session id
    thread->report_connection_check_in(connection_id, session_id);
  }

  void session::check_in() {
    CORE_LOG_DEBUG("Session {} checking in", session_id);
    state_machine.handle_event(network::SESSION_EVENT_START, this);

    active_protocol_handler = make_scope<client_check_in_handler>(this);
    CORE_LOG_DEBUG("Launching [{}] protocol handler for session {}", active_protocol_handler->get_protocol_id(), session_id);
  }

  void session::shutdown() {
    try {
      socket.close();
      state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_START, this);
    } catch (...) {
    }
  }

  void session::finalize() {
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

    uint16_t message_size = static_cast<uint16_t>(msg.data.size());
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&message_size);
    msg_bytes.append_range(std::span(header_bytes, sizeof(message_header)));
    msg_bytes.append_range(std::span(size_bytes, sizeof(uint16_t)));
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

      write_queue.push_back(std::move(chunk));
    } while (offset < msg_bytes.size());
    if (writing) {
      return;
    }

    writing = true;

    auto write_data = std::move(write_queue.front());
    write_queue.pop_front();

    std::ranges::fill(write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    std::span write_buffer_span(write_buffer.data(), write_data.size());
    dump_bytes_for_debug(write_buffer_span, std::format("writing {} bytes", write_data.size()));
    socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&session::finish_write, this));
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

  natural_t session::set_timeout(seconds duration, bool repeating, void (session::*callback)()) {
    natural_t timeout_id = get_timeout_id();
    if (timeouts.find(timeout_id) != timeouts.end()) {
      CORE_LOG_ERROR("Timeout ID collision for session {}", session_id);
      return 0;
    }

    set_timeout(timeout_id, duration, repeating, callback);
    return timeout_id;
  }

  void session::cancel_timeout(natural_t id) {
    auto itr = timeouts.find(id);
    if (itr != timeouts.end()) {
      itr->second.timer.cancel();
      timeouts.erase(itr);
    }
  }

  void session::on_heartbeat_timeout() {
    message ping_msg;
    ping_msg.header = {
      .category = CONTROL,
      .id = PING,
    };

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    ping_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));

    start_write(std::move(ping_msg));

    heartbeat_timeout_id = set_timeout(seconds(1), false, &session::missed_heartbeat_response);
  }

  void session::missed_heartbeat_response() {
    CORE_LOG_WARN("Session {} missed heartbeat PONG response", session_id);
    thread->report_connection_closed(connection_id, session_id);
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
    write_queue.pop_front();

    std::ranges::fill(write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), write_buffer.begin());

    std::span write_buffer_span(write_buffer.data(), write_data.size());
    dump_bytes_for_debug(write_buffer_span, std::format("writing {} bytes", write_data.size()));
    socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&session::finish_write, this));
  }

  void session::poll() {
    std::vector<uint8_t> data = {};
    do {
      if (active_protocol_handler) {
        if (active_protocol_handler->poll()) {
          CORE_LOG_DEBUG("Session {} protocol handler completed : [{}]", session_id, active_protocol_handler->get_protocol_id());
          active_protocol_handler = nullptr;
        }
      } else {
        opt<message> opt_msg = receive_next_message();
        if (opt_msg.has_value()) {
          process_message(std::move(*opt_msg));
        }
      }
    } while (!data.empty());
  }

  std::vector<uint8_t> session::try_receive() {
    if (read_queue.empty()) {
      return {};
    }

    std::vector<uint8_t> data = std::move(read_queue.front());
    read_queue.pop_front();

    if (data.size() < sizeof(message_header) + sizeof(uint16_t)) {
      if (read_queue.empty()) {
        read_queue.push_front(std::move(data));
        return {};
      }

      do {
        std::vector<uint8_t> next_chunk = std::move(read_queue.front());
        read_queue.pop_front();
        data.append_range(next_chunk);
      } while (data.size() < sizeof(message_header) + sizeof(uint16_t) && !read_queue.empty());
      if (data.size() < sizeof(message_header) + sizeof(uint16_t)) {
        read_queue.push_front(std::move(data));
        return {};
      }

      uint16_t message_size = *reinterpret_cast<const uint16_t*>(std::span(data).subspan(sizeof(message_header)).data());
      while (data.size() < sizeof(message_header) + sizeof(uint16_t) + message_size && !read_queue.empty()) {
        std::vector<uint8_t> next_chunk = std::move(read_queue.front());
        read_queue.pop_front();
        data.append_range(next_chunk);
      }
      if (data.size() < sizeof(message_header) + sizeof(uint16_t) + message_size) {
        read_queue.push_front(std::move(data));
        return {};
      }
    }

    return data;
  }

  opt<message> session::receive_next_message() {
    auto data = try_receive();
    if (data.empty()) {
      return std::nullopt;
    }

    message msg;
    msg.header = *reinterpret_cast<message_header*>(data.data());
    msg.data.append_range(std::span(data).subspan(sizeof(message_header) + sizeof(uint16_t)));

    dump_bytes_for_debug(std::span(data), std::format("[RX MESSAGE {}] {}", session_id, msg.header));
    return msg;
  }

  void session::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    reading = false;
    if (state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN || state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      if (state_machine.get_current_state() != network::SESSION_STATE_STOPPED) {
        state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
        thread->report_connection_closed(connection_id, session_id);
      }
      return;
    }

    if (ec) {
      CORE_LOG_TRACE("Read error on session {}: {}", session_id, ec.message());
      thread->report_connection_closed(connection_id, session_id);
      return;
    }

    CORE_LOG_TRACE("Session {} finished reading {} bytes", session_id, bytes_transferred);
    std::vector<uint8_t> data(read_buffer.begin(), read_buffer.begin() + bytes_transferred);
    dump_bytes_for_debug(data, "adding to read queue");
    read_queue.push_back(data);

    start_read();
  }

  void session::finish_write(const asio::error_code& ec, std::size_t bytes_transferred) {
    writing = false;
    if (state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN || state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      if (state_machine.get_current_state() != network::SESSION_STATE_STOPPED) {
        state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
        thread->report_connection_closed(connection_id, session_id);
      }
      return;
    }

    if (ec && (ec == asio::error::operation_aborted || ec == asio::error::eof || ec == asio::error::connection_reset
#if 0
      || ec == asio::error::interrupted
#endif
              )) {
      // Connection closed cleanly by peer.
      thread->report_connection_closed(connection_id, session_id);
      return;
    }

    if (!ec) {
      /// \todo successful write callback ?
      start_write();
    } else {
      /// report error here
    }
  }

  void session::process_message(message&& msg) {
    switch (msg.header.category) {
      case CONTROL:
        switch (msg.header.id) {
          case PING: handle_heartbeat_ping(std::move(msg)); return;
          case PONG: handle_heartbeat_pong(std::move(msg)); return;
          default: break;
        }
        break;

      default: break;
    }

    message session_data;
    session_data.header = {
      .category = SESSION_EVENT,
      .id = SESSION_RX_MESSAGE,
    };

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    const uint8_t* msg_header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);

    session_data.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    session_data.data.append_range(std::span(msg_header_bytes, sizeof(message_header)));
    session_data.data.append_range(std::span(msg.data.data(), msg.data.size()));

    thread->get_message_bus().send_message(std::move(session_data));
  }

  void session::handle_heartbeat_ping(message&& msg) {
    last_heartbeat_time = sys_clock::now();

    message pong_msg;
    pong_msg.header = {
      .category = CONTROL,
      .id = PONG,
    };

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    pong_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));

    start_write(std::move(pong_msg));
  }

  void session::handle_heartbeat_pong(message&& msg) {
    last_heartbeat_time = sys_clock::now();

    if (heartbeat_timeout_id) {
      cancel_timeout(*heartbeat_timeout_id);
      heartbeat_timeout_id = std::nullopt;

      set_timeout(seconds(10), false, &session::on_heartbeat_timeout);
    } else {
      natural_t session_id_in_msg = *reinterpret_cast<const integer_t*>(msg.data.data());
      if (session_id_in_msg != session_id) {
        CORE_LOG_WARN("Session {} received unexpected heartbeat PONG for session {}", session_id, session_id_in_msg);
      } else {
        CORE_LOG_WARN("Session {} received unexpected heartbeat PONG", session_id);
      }
    }
  }

  void session::handle_request_session_information(message&& msg) {
    message rx_msg;
    rx_msg.header = {
      .category = SESSION_EVENT,
      .id = SESSION_RX_MESSAGE,
    };

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    rx_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));

    const uint8_t* msg_header_bytes = reinterpret_cast<const uint8_t*>(&msg.header);
    rx_msg.data.append_range(std::span(msg_header_bytes, sizeof(message_header)));
    rx_msg.data.append_range(std::span(msg.data.data(), msg.data.size()));
    thread->get_message_bus().send_message(std::move(rx_msg));
  }

  void session::set_timeout(natural_t id, seconds duration, bool repeating, void (session::*callback)()) {
    auto itr = timeouts.find(id);
    if (itr == timeouts.end()) {
      timeout to{
        .id = id,
        .timer = asio::steady_timer(io_context, duration),
        .repeating = repeating,
        .callback = callback,
      };

      bool success = false;
      std::tie(itr, success) = timeouts.insert({ id, std::move(to) });
      if (!success) {
        CORE_LOG_ERROR("Failed to insert timeout for session {}", session_id);
        return;
      }
    }
    OTHER_ASSERT(itr != timeouts.end(), "Timeout not found for session {}", session_id);
    itr->second.timer.async_wait(std::bind_front(&session::handle_timeout, this, id));
  }

  void session::handle_timeout(natural_t id, const asio::error_code& ec) {
    if (!ec) {
      auto itr = timeouts.find(id);
      if (itr != timeouts.end()) {
        (this->*itr->second.callback)();

        if (itr->second.repeating) {
          itr->second.timer.expires_after(itr->second.timer.expiry() - asio::steady_timer::clock_type::now());
          itr->second.timer.async_wait(std::bind_front(&session::handle_timeout, this, id));
        } else {
          timeouts.erase(itr);
        }
      }
    }
  }

}  // namespace other