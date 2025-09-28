/**
 * \file server_dev/other_client.hpp
 **/
#ifndef OTHER_SERVER_DEV_OTHER_CLIENT_HPP
#define OTHER_SERVER_DEV_OTHER_CLIENT_HPP

#include <array>
#include <queue>
#include <vector>

#include <asio/asio.hpp>

#include "core/logger.hpp"

namespace other {

  class network_thread;

  class client {
   public:
    client(network_thread* thread, natural_t id, asio::io_context& context, asio::ip::tcp::socket&& socket)
        : client_id(id), io_context(context), socket(std::move(socket)), thread(thread) {
      OTHER_ASSERT(thread != nullptr, "Network thread is null");
      start_read();
    }

    void shutdown();

    void start_read();
    void start_write(const std::vector<uint8_t>& data);

    constexpr static inline size_t kBufferSize = 4096;

    natural_t client_id = 0;

   protected:
    asio::io_context& io_context;
    asio::ip::tcp::socket socket;

    network_thread* thread;

    bool reading = false;
    std::array<uint8_t, kBufferSize> read_buffer{};
    std::queue<std::vector<uint8_t>> read_queue{};

    bool writing = false;
    std::array<uint8_t, kBufferSize> write_buffer{};
    std::queue<std::vector<uint8_t>> write_queue{};

    void start_write();

    void finish_read(const asio::error_code& ec, std::size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, std::size_t bytes_transferred);
  };

}  // namespace other

#endif  // OTHER_SERVER_DEV_OTHER_CLIENT_HPP