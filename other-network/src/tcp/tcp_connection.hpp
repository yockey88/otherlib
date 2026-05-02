/**
 * \file tcp/tcp_connection.hpp
 **/
#ifndef OTHER_NETWORK_TCP_TCP_CONNECTION_HPP
#define OTHER_NETWORK_TCP_TCP_CONNECTION_HPP

#include "network/connection.hpp"

namespace other {

  class tcp_connection : public connection {
   public:
    tcp_connection() = default;
    virtual ~tcp_connection() = default;

   private:
  };

}  // namespace other

#endif  // OTHER_NETWORK_TCP_TCP_CONNECTION_HPP