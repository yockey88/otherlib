/**
 * \file network/session.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_SESSION_HPP
#define OTHER_NETWORK_NETWORK_SESSION_HPP

#include <array>
#include <queue>
#include <vector>

#include <asio/asio.hpp>

#include "core/logger.hpp"
#include "thread/message.hpp"

#include "network/session_state_machine.hpp"

namespace other {

  class network_thread;

  class session {
   public:
    session(network_thread* thread, integer_t id, asio::io_context& context)
        : session_id(id), socket(context), io_context(context), thread(thread) {
    }
    session(network_thread* thread, integer_t id, asio::io_context& context, asio::ip::tcp::socket&& socket)
        : session_id(id), socket(std::move(socket)), io_context(context), thread(thread) {
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

    void poll();
    std::vector<uint8_t> try_receive();

    constexpr static inline size_t kBufferSize = 4096;

    integer_t session_id = 0;
    asio::ip::tcp::socket socket;

   protected:
    asio::io_context& io_context;

    network_thread* thread;

    bool reading = false;
    std::array<uint8_t, kBufferSize> read_buffer{};
    std::queue<std::vector<uint8_t>> read_queue{};

    bool writing = false;
    std::array<uint8_t, kBufferSize> write_buffer{};
    std::queue<std::vector<uint8_t>> write_queue{};

    session_state_machine state_machine;

    void start_write();

    void finish_read(const asio::error_code& ec, std::size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, std::size_t bytes_transferred);

    void handle_control_ping(const message_header& header, const std::span<uint8_t> data);
    void handle_control_pong(const message_header& header, const std::span<uint8_t> data);

    void dump_bytes_for_debug(const std::span<uint8_t> data, const std::string_view msg = "");
    void dump_message_bytes(const message& msg);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_SESSION_HPP