/**
 * \file http-server.hpp
 **/
#ifndef OTHER_SERVER_HTTP_SERVER_HPP
#define OTHER_SERVER_HTTP_SERVER_HPP

#include "network/connection_handler.hpp"

namespace other {

  class server;

  class http_server {
   public:
    http_server(server* srv_ptr, uint16_t port);
    ~http_server() = default;

    void process_new_connection(natural_t connection_id);

   private:
    uint16_t port;
    natural_t main_listening_id = 0;

    server* server_ptr = nullptr;

    std::vector<natural_t> connections;
  };

}  // namespace other

#endif  // OTHER_SERVER_HTTP_SERVER_HPP
