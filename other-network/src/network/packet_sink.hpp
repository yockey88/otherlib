/**
 * \file network/packet_sink.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_PACKET_SINK_HPP
#define OTHER_NETWORK_NETWORK_PACKET_SINK_HPP

#include <span>

#include "core/defines.hpp"
#include "core/interfaces.hpp"

namespace other {

  /// a byte endpoint on a transport: the link (link_sink) or a boundary observer (taps,
  ///  tooling). called on the owning provider's home thread — implementations marshal
  ///  themselves if they need another one
  class OTHER_CLASS packet_sink {
    OTHER_ENVIRONMENT_INTERFACE("Network", "PacketSink");

   public:
    virtual ~packet_sink() = default;

    virtual void rx_data(natural_t conn_id, std::span<const uint8_t> data) = 0;
    virtual void connection_opened(natural_t conn_id) {}
    virtual void connection_closed(natural_t conn_id) {}
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_PACKET_SINK_HPP
