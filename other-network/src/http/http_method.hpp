/**
 * \file http/http_method.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_HTTP_METHOD_HPP
#define OTHER_NETWORK_HTTP_HTTP_METHOD_HPP

#include <array>
#include <string>
#include <string_view>

namespace other {
  namespace http {

    enum class verb : uint8_t {
      HTTP_GET = 0,
      HTTP_POST,
      HTTP_PUT,
      HTTP_DELETE,
      HTTP_PATCH,
      HTTP_HEAD,
      HTTP_OPTIONS,

      NUM_HTTP_METHODS,
    };
    constexpr static size_t kNumHttpMethods = static_cast<size_t>(verb::NUM_HTTP_METHODS);

    constexpr static std::array<std::string_view, kNumHttpMethods> kHttpMethodNames{
      "GET",
      "POST",
      "PUT",
      "DELETE",
      "PATCH",
      "HEAD",
      "OPTIONS",
    };

    struct method_info {
      verb method = verb::HTTP_GET;
      std::string name{ kHttpMethodNames[0] };

      method_info() = default;
      method_info(verb method) : method(method), name(kHttpMethodNames[static_cast<size_t>(method)]) {}
    };

    static inline std::array<method_info, kNumHttpMethods> http_methods{
      method_info{ verb::HTTP_GET },
      method_info{ verb::HTTP_POST },
      method_info{ verb::HTTP_PUT },
      method_info{ verb::HTTP_DELETE },
      method_info{ verb::HTTP_PATCH },
      method_info{ verb::HTTP_HEAD },
      method_info{ verb::HTTP_OPTIONS },
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_HTTP_METHOD_HPP