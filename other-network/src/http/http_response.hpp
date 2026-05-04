/**
 * \file http/http_response.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_HTTP_RESPONSE_HPP
#define OTHER_NETWORK_HTTP_HTTP_RESPONSE_HPP

#include <cstdint>
#include <vector>

#include "http/http_header.hpp"
#include "http/version.hpp"

namespace other {
  namespace http {

    struct response {
      uint16_t status_code;
      std::vector<header> headers;
      std::vector<uint8_t> body;

      response() = default;
      response(uint16_t status_code) : status_code(status_code) {}
      response(uint16_t status_code, std::vector<uint8_t> body)
          : status_code(status_code), body(std::move(body)) {}
      response(uint16_t status_code, std::vector<header> headers, std::vector<uint8_t> body)
          : status_code(status_code), headers(std::move(headers)), body(std::move(body)) {}

      void set_body(const std::string_view content, const std::string_view content_type = "text/html");
      std::vector<uint8_t> serialize(http::version version) const;

      std::string get_response_string(http::version version) const;

     private:
      std::string status_message() const;
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_RESPONSE_HPP