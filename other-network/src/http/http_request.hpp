/**
 * \file http/http_request.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_HTTP_REQUEST_HPP
#define OTHER_NETWORK_HTTP_HTTP_REQUEST_HPP

#include <cstdint>
#include <string>

#include "http/http_header.hpp"
#include "http/http_method.hpp"

namespace other {
  namespace http {

    struct request {
      http::method_info method;

      std::string path;
      std::string query_string;

      ostd::vector<header> headers;
      ostd::vector<uint8_t> body;
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_REQUEST_HPP