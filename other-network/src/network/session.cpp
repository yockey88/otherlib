/**
 * \file network/session.cpp
 **/
#include "network/session.hpp"

#include <algorithm>
#include <cstdint>

#include <asio/asio/error.hpp>

#include "thread/message.hpp"

#include "network/message.hpp"
#include "network/network_thread.hpp"
#include "network/protocols/check_in_protocol.hpp"
#include "network/session_state_machine.hpp"

namespace other {

  session::session(session& other)
      : socket(std::move(other.socket)), connection_timer(std::move(other.connection_timer)), io_context(other.io_context) {
    thread = other.thread;
    other.thread = nullptr;

    state_machine = other.state_machine;
    other.state_machine = {};

    buffer.reading = other.buffer.reading;
    buffer.read_queue = std::move(other.buffer.read_queue);
    buffer.read_buffer = std::move(other.buffer.read_buffer);

    buffer.writing = other.buffer.writing;
    buffer.write_queue = std::move(other.buffer.write_queue);
    buffer.write_buffer = std::move(other.buffer.write_buffer);
  }

  void session::start_initialization() {
    CORE_LOG_DEBUG("Session {} starting initialization", session_id);
    state_machine.handle_event(network::SESSION_EVENT_START, this);

    active_protocol_handler = make_scope<server_check_in_handler>(this);
    active_protocol_handler->begin_protocol();
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
    CORE_LOG_DEBUG("Session {} starting shutdown", session_id);

    try {
      state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_START, this);
      socket.close();
    } catch (...) {
    }

    if (!buffer.reading && !buffer.writing) {
      CORE_LOG_DEBUG(" > Session inactive, shutting down immediately");
      thread->report_connection_closed(connection_id, session_id);
    }
  }

  void session::finalize() {
    state_machine.handle_event(network::SESSION_EVENT_SHUTDOWN_COMPLETE, this);
  }

  void session::start_read() {
    if (buffer.reading) {
      return;
    }
    buffer.reading = true;

    socket.async_receive(asio::buffer(buffer.read_buffer), std::bind_front(&session::finish_read, this));
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
    /// count misses
  }

  void session::start_write() {
    if (buffer.writing) {
      return;
    }
    if (buffer.write_queue.empty()) {
      return;
    }

    buffer.writing = true;
    auto write_data = std::move(buffer.write_queue.front());
    buffer.write_queue.pop_front();

    std::ranges::fill(buffer.write_buffer, 0);
    std::ranges::copy(write_data.begin(), write_data.end(), buffer.write_buffer.begin());

    std::span write_buffer_span(buffer.write_buffer.data(), write_data.size());
    dump_bytes_for_debug(write_buffer_span, std::format("[TX CHUNK ({} bytes)]", write_data.size()));
    socket.async_send(asio::buffer(write_buffer_span), std::bind_front(&session::finish_write, this));
  }

  natural_t session::send_and_wait_response(message&& msg, message_header expected_response, microseconds timeout_duration, response_callback callback) {
    natural_t response_id = next_response_id++;
    if (responses.find(response_id) != responses.end()) {
      CORE_LOG_ERROR("Response ID collision for session {}", session_id);
      return 0;
    }

    response resp(thread->get_io_context());
    resp.expected_header = expected_response;
    resp.callback = callback;
    resp.timer = asio::steady_timer(io_context);
    resp.timer.expires_after(timeout_duration);
    resp.timer.async_wait([this, response_id](const asio::error_code& ec) {
      if (ec) {
        return;
      }

      auto itr = responses.find(response_id);
      if (itr != responses.end()) {
        CORE_LOG_WARN("Response ID {} timed out for session {}", response_id, session_id);

        responses.erase(itr);
      }
    });
    responses.insert({ response_id, std::move(resp) });

    start_write(std::move(msg));
    return response_id;
  }

  void session::cancel_response(natural_t response_id) {
    auto itr = responses.find(response_id);
    if (itr != responses.end()) {
      itr->second.timer.cancel();
      responses.erase(itr);
    }
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

  opt<message> session::receive_next_message() {
    auto data = try_receive();
    if (data.empty()) {
      return std::nullopt;
    }

    auto bytes = std::span(data);

    message msg;
    msg.header = *reinterpret_cast<message_header*>(bytes.data());
    bytes = bytes.subspan(sizeof(message_header));

    uint16_t size = *reinterpret_cast<const uint16_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(uint16_t));

    //// this MUST have been validated in try_receive
    /// TODO: at this point, the message should be EQUAL in size, make this assert ==
    OTHER_ASSERT(bytes.size() >= size, "Received message data size is smaller than expected");

    msg.data.append_range(bytes.subspan(0, size));

    dump_bytes_for_debug(std::span(data), std::format("[RX MESSAGE {}] {}", session_id, msg.header));
    return msg;
  }

  void session::finish_read(const asio::error_code& ec, std::size_t bytes_transferred) {
    if (state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      return;
    }

    buffer.reading = false;
    bool should_close = false;
    if ((ec && (ec == asio::error::operation_aborted || ec == asio::error::eof || ec == asio::error::connection_aborted || ec == asio::error::connection_reset)) ||
        state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN) {
      if (ec && (ec == asio::error::eof || ec == asio::error::connection_aborted || ec == asio::error::connection_reset)) {
        CORE_LOG_INFO("Session {} connection closed by peer", session_id);
      }

      should_close = true;
    }

    if (should_close) {
      thread->report_connection_closed(connection_id, session_id);
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Read error on session {}: {}", session_id, ec.message());
      /// \todo: should report and try to fix (or kill session if need be), for now we just close
      // thread->report_error(connection_id, session_id, ec);
      state_machine.handle_event(network::SESSION_EVENT_CLOSE_ON_ERROR, this);
      thread->report_connection_closed(connection_id, session_id);
    } else {
      CORE_LOG_TRACE("Session {} finished reading {} bytes", session_id, bytes_transferred);
      std::vector<uint8_t> data(buffer.read_buffer.begin(), buffer.read_buffer.begin() + bytes_transferred);

      dump_bytes_for_debug(data, std::format("[RX CHUNK {} bytes]", data.size()));
      buffer.read_queue.push_back(data);
    }

    start_read();
  }

  void session::finish_write(const asio::error_code& ec, std::size_t bytes_transferred) {
    if (state_machine.get_current_state() == network::SESSION_STATE_STOPPED) {
      return;
    }

    buffer.writing = false;
    if ((ec && ec == asio::error::operation_aborted) || state_machine.get_current_state() == network::SESSION_STATE_SHUTTING_DOWN) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Write error on session {}: {}", session_id, ec.message());
      /// \todo:
      thread->report_connection_error(this, ec);
    } else {
      /// \todo successful write callback ?
      CORE_LOG_TRACE("Session {} finished writing {} bytes", session_id, bytes_transferred);
      start_write();
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

    /// search pending responses first
    auto itr = std::ranges::find_if(responses, [&](const auto& pair) {
      return pair.second.expected_header.category == msg.header.category && pair.second.expected_header.id == msg.header.id;
    });
    if (itr != responses.end()) {
      itr->second.timer.cancel();
      itr->second.callback(std::move(msg));
      responses.erase(itr);
      return;
    }

    message session_data;
    session_data.header = {
      .category = SESSION_EVENT,
      .id = SESSION_RX_MESSAGE,
    };
    session_event_rx_message session_msg;
    session_msg.session_id = session_id;
    session_msg.msg = std::move(msg);
    session_data.data.append_range(session_msg.as_buffer());

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