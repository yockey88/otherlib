/**
 * \file network/udp_stream.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_UDP_STREAM_HPP
#define OTHER_NETWORK_NETWORK_UDP_STREAM_HPP

#include <queue>
#include <type_traits>

#include <asio/asio.hpp>
#include <asio/asio/ip/udp.hpp>

#include "core/async_buffer.hpp"
#include "core/coroutine.hpp"
#include "thread/message.hpp"
#include "thread/test/new_message.hpp"

namespace other {

  class udp_stream;
  class network_thread;

  struct udp_handle {
    void send(const std::span<const uint8_t> data);
    void send(message&& msg);

    template <typename T>
      requires std::is_trivially_copyable_v<T>
    void send(T&& obj) {
      const uint8_t* obj_bytes = reinterpret_cast<const uint8_t*>(&obj);
      send(std::span<const uint8_t>(obj_bytes, sizeof(T)));
    }

    std::vector<uint8_t> receive();

    std::mutex* stream_mutex = nullptr;
    udp_stream* stream = nullptr;
  };

  class udp_stream {
   public:
    udp_stream(network_thread* thread, natural_t connection_id, integer_t stream_id, asio::io_context& io_context, const asio::ip::udp::endpoint& endpoint, const asio::ip::udp::endpoint& remote_endpoint);

    udp_stream(const udp_stream&) = delete;
    udp_stream& operator=(const udp_stream&) = delete;

    ~udp_stream() = default;

    void poll();

    void start_read();
    void shutdown();

    std::vector<uint8_t> receive();
    void send(const std::span<const uint8_t> data);
    void send(message&& msg);

    inline udp_handle* get_handle() {
      return &this_handle;
    }

    constexpr static size_t kBufferSize = 4096;

    asio::ip::udp::socket socket;
    asio::ip::udp::endpoint endpoint;
    asio::ip::udp::endpoint remote_endpoint;

    natural_t connection_id = 0;
    integer_t stream_id = 0;

   private:
    asio::io_context& io_context;
    network_thread* thread = nullptr;

    std::queue<std::vector<uint8_t>> full_data_queue;
    async_buffer<kBufferSize> buffer;

    std::mutex stream_mutex;
    udp_handle this_handle;

    void start_write();
    void start_write(const std::span<const uint8_t> data);

    void finish_read(const asio::error_code& ec, std::size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, std::size_t bytes_transferred);

    std::vector<uint8_t> try_receive();

    void dump_bytes_for_debug(const std::span<uint8_t> data, const std::string_view msg);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_UDP_STREAM_HPP