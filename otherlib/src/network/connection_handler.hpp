/**
 * \file network/connection_handler.hpp
 **/
#ifndef OTHER_NETWORK_CONNECTION_HANDLER_HPP
#define OTHER_NETWORK_CONNECTION_HANDLER_HPP

#include "core/defines.hpp"
#include "thread/message.hpp"

namespace other {

  class connection_handler {
   public:
    virtual ~connection_handler() = default;

    virtual void on_accept_tcp_connection(const binding_point& endpoint, natural_t connection_id) {}
    virtual void on_establish_tcp_connection(const binding_point& endpoint, natural_t connection_id) {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_CONNECTION_HANDLER_HPP