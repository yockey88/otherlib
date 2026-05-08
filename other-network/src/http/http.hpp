/**
 * \file http/http.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_HTTP_HPP
#define OTHER_NETWORK_HTTP_HTTP_HPP

#include "core/defines.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"

namespace other {
  namespace http {

    bool is_http_request(const std::span<const uint8_t> data);

    opt<request> parse_http_request(const std::span<const uint8_t> raw_request);

    response internal_error(const std::string_view error_message);

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_HPP