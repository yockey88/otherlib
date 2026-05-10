/**
 * \file network/transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "thread/message.hpp"

namespace other {

  class transport_provider {
   public:
    virtual ~transport_provider() = default;

    virtual std::string_view name() const = 0;

    virtual void initialize(asio::io_context& net_io) = 0;
    virtual void tick() = 0;
    virtual void shutdown() = 0;

    virtual natural_t start_listen(const binding_point& endpoint) = 0;
    virtual natural_t start_connect(const binding_point& endpoint) = 0;
    virtual void tx(natural_t connection_id, std::span<const uint8_t> data) = 0;
    virtual void close(natural_t connection_id) = 0;

    virtual bool is_reliable() const { return true; }
    virtual bool is_ordered() const { return true; }
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_TRANSPORT_PROVIDER_HPP