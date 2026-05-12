/**
 * \file http/http_header.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_HTTP_HEADER_HPP
#define OTHER_NETWORK_HTTP_HTTP_HEADER_HPP

#include <string>

namespace other {
  namespace http {

    struct header {
      std::string name;
      std::string value;
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_HEADER_HPP