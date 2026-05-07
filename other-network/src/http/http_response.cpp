/**
 * \file http/http_response.cpp
 **/
#include "http/http_response.hpp"

#include <format>
#include <string>

namespace other {
  namespace http {

    void response::set_headers(const std::vector<header>& headers) {
      this->headers = headers;
    }

    void response::add_header(const header& header) {
      for (auto& existing_header : headers) {
        if (existing_header.name == header.name) {
          existing_header.value = header.value;
          return;
        }
      }
      headers.push_back(header);
    }

    void response::set_body(const std::vector<uint8_t>& body, const std::string_view content_type) {
      if (auto itr = std::ranges::find_if(headers, [&](const header& h) { return h.name == "Content-Type"; });
          itr != headers.end()) {
        headers.erase(itr);
      }
      if (auto itr = std::ranges::find_if(headers, [&](const header& h) { return h.name == "Content-Length"; });
          itr != headers.end()) {
        headers.erase(itr);
      }
      add_header({ "Content-Type", std::string(content_type) });
      add_header({ "Content-Length", std::to_string(body.size()) });
      this->body = body;
    }

    void response::set_body_content(const std::string_view content, const std::string_view content_type) {
      std::vector<uint8_t> body_bytes(content.begin(), content.end());
      set_body(body_bytes, content_type);
    }

    std::string response::get_response_string(http::version ver) const {
      std::string http_version_str = std::format("HTTP/{}.{}", ver.major, ver.minor);
      std::string status_line = std::format("{} {} {}\r\n", http_version_str, status_code, status_message());
      std::string headers_str;
      for (const auto& header : headers) {
        headers_str += std::format("{}: {}\r\n", header.name, header.value);
      }
      return status_line + headers_str + "\r\n" + std::string(body.begin(), body.end());
    }

    std::vector<uint8_t> response::serialize(http::version ver) const {
      std::string resp_str = get_response_string(ver);

      const uint8_t* resp_bytes = reinterpret_cast<const uint8_t*>(resp_str.data());
      return std::vector<uint8_t>{ resp_bytes, resp_bytes + resp_str.size() };
    }

    std::string response::status_message() const {
      if (status_code >= 100 && status_code < 200) {
        return "Informational";
      } else if (status_code >= 200 && status_code < 300) {
        return "OK";
      } else if (status_code >= 300 && status_code < 400) {
        return "Redirection";
      } else if (status_code >= 400 && status_code < 500) {
        return "Client Error";
      } else if (status_code >= 500 && status_code < 600) {
        return "Server Error";
      } else {
        return "Unknown Status";
      }
    }

  }  // namespace http
}  // namespace other