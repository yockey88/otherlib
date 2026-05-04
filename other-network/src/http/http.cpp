/**
 * \file http/http.cpp
 **/
#include "http/http.hpp"

#include "core/logger.hpp"

#include "http/http_method.hpp"
#include "network/network_error.hpp"

namespace other {
  namespace detail {

    http::method_info parse_http_method(const std::span<const uint8_t> bytes) {
      std::string_view method_str(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      for (const auto& method : http::http_methods) {
        if (method_str.starts_with(method.name)) {
          return method;
        }
      }
      throw http_method_parse_error(std::format("Unsupported HTTP method: {}", method_str));
    }

  }  // namespace detail
  namespace http {

    opt<request> parse_http_request(const std::span<const uint8_t> raw_request) {
      request req;

      auto fail = [&](const std::string_view error_message) -> opt<http::request> {
        CORE_LOG_ERROR("Failed to parse HTTP request: {}", error_message);
        return std::nullopt;
      };

      try {
        req.method = detail::parse_http_method(raw_request);
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
      response.set_body(std::string(error_message));
      return response;
    }

  }  // namespace http
}  // namespace other