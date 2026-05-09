/**
 * \file network/io.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_IO_HPP
#define OTHER_NETWORK_NETWORK_IO_HPP

#include <asio/asio.hpp>

namespace other {

  struct io {
    constexpr static size_t kDefaultIoThreadPoolSize = 4;

    asio::io_context context;
    asio::thread_pool thread_pool;

    io() : thread_pool(kDefaultIoThreadPoolSize) {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_IO_HPP