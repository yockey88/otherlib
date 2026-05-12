/**
 * \file network/network_error.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_ERROR_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_ERROR_HPP

#include <stdexcept>

namespace other {

  struct network_error : public std::runtime_error {
    network_error() : std::runtime_error("Network error") {}
    explicit network_error(const std::string& message)
        : std::runtime_error(message) {}
  };

  struct invalid_provider_network_error : public network_error {
    explicit invalid_provider_network_error(const std::string& message)
        : network_error(message) {}
  };

  struct port_in_use_network_error : public network_error {
    explicit port_in_use_network_error(const std::string& message)
        : network_error(message) {}
  };

  struct http_method_parse_error : public network_error {
    explicit http_method_parse_error(const std::string& message)
        : network_error(message) {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_NETWORK_ERROR_HPP
