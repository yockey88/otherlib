/**
 * \file http/http.cpp
 **/
#include "http/http.hpp"

#include "core/logger.hpp"

#include "http/http_method.hpp"
#include "network/network_error.hpp"

namespace other {
  namespace detail {

    http::method_info parse_http_method(std::span<const uint8_t>& bytes);
    std::string_view parse_http_path(std::span<const uint8_t>& bytes);
    std::string_view parse_http_query_string(std::span<const uint8_t>& bytes);
    std::vector<http::header> parse_headers(std::span<const uint8_t>& bytes);

  }  // namespace detail
  namespace http {

    opt<request> parse_http_request(const std::span<const uint8_t> raw_request) {
      request req;

      auto fail = [&](const std::string_view error_message) -> opt<http::request> {
        CORE_LOG_ERROR("Failed to parse HTTP request: {}", error_message);
        return std::nullopt;
      };

      try {
        std::span bytes = raw_request;
        req.method = detail::parse_http_method(bytes);

        // skip to non-whitespace after method
        while (!bytes.empty() && (bytes[0] == ' ' || bytes[0] == '\r' || bytes[0] == '\n')) {
          bytes = bytes.subspan(1);
        }
        if (bytes.empty()) {
          return fail("Unexpected end of request after HTTP method");
        }

        if (bytes[0] != '/') {
          return fail("Invalid HTTP request: path must start with '/'");
        }
        bytes = bytes.subspan(1);  // skip initial '/'

        if (bytes[0] != ' ') {
          // parse path
          req.path = detail::parse_http_path(bytes);
        } else {
          req.path = "/";
        }

        if (bytes[0] == '?') {
          req.query_string = detail::parse_http_query_string(bytes);
        }

        // req.headers = detail::parse_headers(bytes);
        req.body = std::vector<uint8_t>(bytes.begin(), bytes.end());
      } catch (const http_method_parse_error& e) {
        return fail(std::format("Failed to parse HTTP method: {}", e.what()));
      } catch (const std::exception& e) {
        return fail(std::format("Error parsing HTTP request: {}", e.what()));
      } catch (...) {
        return fail("Unknown error parsing HTTP request");
      }

      return req;
    }

    bool is_http_request(const std::span<const uint8_t> data) {
      if (data.size() < 4) {
        return false;
      }

      std::string str(reinterpret_cast<const char*>(data.data()), data.size());
      for (const auto& method : http::http_methods) {
        if (str.starts_with(method.name)) {
          return true;
        }
      }
      return false;
    }

    response internal_error(const std::string_view error_message) {
      response response;
      response.status_code = 500;
      response.set_body_content(std::string(error_message), "text/plain");
      return response;
    }

  }  // namespace http
  namespace detail {

    http::method_info parse_http_method(std::span<const uint8_t>& bytes) {
      std::string_view method_str(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      for (const auto& method : http::http_methods) {
        if (method_str.starts_with(method.name)) {
          bytes = bytes.subspan(method.name.size());
          return method;
        }
      }
      throw http_method_parse_error(std::format("Unsupported HTTP method: {}", method_str));
    }

    std::string_view parse_http_path(std::span<const uint8_t>& bytes) {
      size_t pos = 0;
      while (pos < bytes.size() && bytes[pos] != ' ' && bytes[pos] != '?') {
        pos++;
      }
      std::string_view path(reinterpret_cast<const char*>(bytes.data()), pos);
      bytes = bytes.subspan(pos);
      return path;
    }

    std::string_view parse_http_query_string(std::span<const uint8_t>& bytes) {
      if (bytes.empty() || bytes[0] != '?') {
        return "";
      }
      bytes = bytes.subspan(1);  // skip '?'

      size_t pos = 0;
      while (pos < bytes.size() && bytes[pos] != ' ') {
        pos++;
      }
      std::string_view query(reinterpret_cast<const char*>(bytes.data()), pos);
      bytes = bytes.subspan(pos);
      return query;
    }

    std::vector<http::header> parse_headers(std::span<const uint8_t>& bytes) {
      std::vector<http::header> headers;
      while (true) {
        size_t pos = 0;
        while (pos < bytes.size() && !(bytes[pos] == '\r' && pos + 1 < bytes.size() && bytes[pos + 1] == '\n')) {
          pos++;
        }
        if (pos == 0) {
          // Reached end of headers
          if (bytes.size() >= 2 && bytes[0] == '\r' && bytes[1] == '\n') {
            bytes = bytes.subspan(2);  // skip "\r\n"
          }
          break;
        }

        std::string_view line(reinterpret_cast<const char*>(bytes.data()), pos);
        bytes = bytes.subspan(pos + 2);  // skip line and "\r\n"

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string_view::npos) {
          throw network_error(std::format("Malformed HTTP header line: '{}'", line));
        }

        std::string_view name = line.substr(0, colon_pos);
        std::string_view value = line.substr(colon_pos + 1);
        // Trim whitespace from value
        value.remove_prefix(std::min(value.find_first_not_of(' '), value.size()));
        value.remove_suffix(std::min(value.size() - value.find_last_not_of(' ') - 1, value.size()));

        headers.push_back({ std::string(name), std::string(value) });
      }
      return headers;
    }

  }  // namespace detail
}  // namespace other