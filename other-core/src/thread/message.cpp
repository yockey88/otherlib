/**
 * \file thread/message.cpp
 **/
#include "thread/message.hpp"

#include <sstream>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  std::string binding_point::write_string(const binding_point& bp) {
    std::stringstream ss;
    ss << "Binding point: ";
    for (integer_t i = 3; i >= 0; --i) {
      ss << std::to_string(bp.bytes[i]);
      if (i != 0) {
        ss << ".";
      }
    }
    ss << ":" << bp.port;
    return ss.str();
  }

  std::string binding_point::write_string(const asio::ip::tcp::endpoint& ep) {
    binding_point bp = from_asio(ep.address(), static_cast<uint16_t>(ep.port()));
    return write_string(bp);
  }

  std::string binding_point::write_string(const asio::ip::udp::endpoint& ep) {
    binding_point bp = from_asio(ep.address(), static_cast<uint16_t>(ep.port()));
    return write_string(bp);
  }

  binding_point binding_point::from_asio(const asio::ip::address& addr, uint16_t port) {
    binding_point bp;
    bp.port = port;
    if (addr.is_v4()) {
      bp.ip = addr.to_v4().to_uint();
    } else {
      OTHER_ASSERT(false, "non IPv4 IP addresses not supported currently");
    }
    return bp;
  }

}  // namespace other