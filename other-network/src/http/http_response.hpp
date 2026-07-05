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

      response() = default;
      response(uint16_t status_code) : status_code(status_code) {}
      response(uint16_t status_code, ostd::vector<uint8_t> body)
          : status_code(status_code), body(std::move(body)) {}
      response(uint16_t status_code, ostd::vector<header> headers, ostd::vector<uint8_t> body)
          : status_code(status_code), headers(std::move(headers)), body(std::move(body)) {}

      void set_headers(const std::span<const header> headers);
      void add_header(const header& header);

      void set_body(const std::span<const uint8_t> body, const std::string_view content_type = "application/octet-stream");
      void set_body_content(const std::string_view content, const std::string_view content_type = "text/plain");
      std::string get_response_string(http::version version) const;

      ostd::vector<uint8_t> serialize(http::version version) const;

     private:
      ostd::vector<header> headers;
      ostd::vector<uint8_t> body;

      std::string status_message() const;
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_RESPONSE_HPP