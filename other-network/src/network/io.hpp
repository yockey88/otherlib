/**
 * \file network/io.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_IO_HPP
#define OTHER_NETWORK_NETWORK_IO_HPP

#include <asio/asio.hpp>

namespace other {

  struct io {
    asio::io_context context;

    io() {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_IO_HPP