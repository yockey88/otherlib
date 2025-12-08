/**
 * \file network/udp_stream.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_UDP_STREAM_HPP
#define OTHER_NETWORK_NETWORK_UDP_STREAM_HPP

#include <queue>

#include <asio/asio.hpp>
#include <asio/asio/ip/udp.hpp>

#include "core/async_buffer.hpp"
#include "thread/message.hpp"
#include "thread/test/new_message.hpp"

namespace other {

  class udp_stream {
   public:
    udp_stream(asio::io_context& io_context, const asio::ip::udp::endpoint& endpoint)
        : io_context(io_context), socket(io_context, endpoint) {
    }

    udp_stream(const udp_stream&) = delete;
    udp_stream& operator=(const udp_stream&) = delete;

    ~udp_stream() = default;

    void start_read();
    void shutdown();

    std::vector<uint8_t> receive();
    void send(const std::span<const uint8_t> data);
    void send(message&& msg);

    constexpr static size_t kBufferSize = 4096;

    asio::ip::udp::socket socket;

   private:
    asio::io_context& io_context;

    std::queue<std::vector<uint8_t>> full_data_queue;
    async_buffer<kBufferSize> buffer;

    void start_write();
    void start_write(const std::span<const uint8_t> data);

    void finish_read(const asio::error_code& ec, std::size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, std::size_t bytes_transferred);

    std::vector<uint8_t> try_receive();
  };

  struct udp_handle {
    void send(const std::span<const uint8_t> data);
    void send(message&& msg);
    std::vector<uint8_t> receive();

    binding_point endpoint;
    std::mutex* stream_mutex = nullptr;
    udp_stream* stream = nullptr;
  };

#pragma pack(push, 1)
  struct new_udp_stream_binding_response : other_message_spec_impl<new_udp_stream_binding_response> {
    udp_handle handle;
  };
#pragma pack(pop)

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_UDP_STREAM_HPP