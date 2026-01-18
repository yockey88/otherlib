/**
 * \file network/session.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_SESSION_HPP
#define OTHER_NETWORK_NETWORK_SESSION_HPP

#include <array>
#include <deque>
#include <vector>

#include <asio/asio.hpp>

#include "core/async_buffer.hpp"
#include "core/timer.hpp"
#include "thread/message.hpp"

#include "network/session_protocol_handler.hpp"
#include "network/session_state_machine.hpp"

namespace other {

  class network_thread;

  class session {
   public:
    session(network_thread* thread, natural_t connection_id, integer_t id, asio::io_context& context)
        : connection_id(connection_id), session_id(id), socket(context), io_context(context), thread(thread) {
    }
    session(network_thread* thread, natural_t connection_id, integer_t id, asio::io_context& context, asio::ip::tcp::socket&& socket)
        : connection_id(connection_id), session_id(id), socket(std::move(socket)), io_context(context), thread(thread) {
    }

    session(session&);
    session& operator=(session&&);

    void start_initialization();
    void checked_in();
    void check_in();

    void shutdown();
    void finalize();

    void start_read();
    void start_write(message&& msg);

    using response_callback = std::function<void(message&&)>;
    natural_t send_and_wait_response(message&& msg, message_header expected_response, microseconds timeout_duration, response_callback callback);
    void cancel_response(natural_t response_id);

    void poll();
    std::vector<uint8_t> try_receive();

    opt<message> receive_next_message();

    network::session_state get_current_state() const {
      return state_machine.get_current_state();
    }

    constexpr static inline size_t kBufferSize = 4096;
    constexpr static inline integer_t kNetworkThreadSessionId = 0;
    constexpr static inline integer_t kInvalidSessionId = -1;

    natural_t connection_id = 0;
    integer_t session_id = kInvalidSessionId;

    asio::ip::tcp::socket socket;
    asio::io_context& io_context;

    network_thread* thread;

    void dump_bytes_for_debug(const std::span<uint8_t> data, const std::string_view msg = "");
    void dump_message_bytes(const message& msg);

    inline natural_t get_timeout_id() {
      static natural_t next_id = 1;
      return next_id++;
    }

    natural_t set_timeout(seconds duration, bool repeating, void (session::*callback)());
    void cancel_timeout(natural_t id);

    void on_heartbeat_timeout();
    void missed_heartbeat_response();

   protected:
    struct timeout {
      natural_t id = 0;
      asio::steady_timer timer;

      bool repeating = false;

      void (session::*callback)();
    };
    std::map<natural_t, timeout> timeouts;

    struct response {
      message_header expected_header;
      response_callback callback;
      asio::steady_timer timer;

      response(asio::io_context& io_ctx)
          : timer(io_ctx) {}
    };
    natural_t next_response_id = 1;
    std::map<natural_t, response> responses;

    async_buffer<kBufferSize> buffer;

    session_state_machine state_machine;

    opt<natural_t> heartbeat_timeout_id = std::nullopt;
    system_timepoint last_heartbeat_time = sys_clock::now();
    scope<protocol_handler> active_protocol_handler = nullptr;

    void start_write();

    void finish_read(const asio::error_code& ec, std::size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, std::size_t bytes_transferred);

    void process_message(message&& msg);

    void handle_heartbeat_ping(message&& msg);
    void handle_heartbeat_pong(message&& msg);

    void handle_request_session_information(message&& msg);

   private:
    void set_timeout(natural_t id, seconds duration, bool repeating, void (session::*callback)());
    void handle_timeout(natural_t id, const asio::error_code& ec);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_SESSION_HPP