/**
 * \file plugins/tcp_side_channel.hpp
 **/
#include "core/profiler.hpp"
#include "driver/driver.hpp"
#include "plugin/plugin.hpp"

#include "network/packet_sink.hpp"

using other::packet_sink;

/// observer sink: called on the provider's home thread (net thread for tcp); the
///  logger is thread-safe, so no marshaling is needed here
class OTHER_CLASS tcp_side_channel : public other::packet_sink {
 public:
  ~tcp_side_channel() override = default;

  void rx_data(other::natural_t conn_id, std::span<const uint8_t> data) override {
    PROFILE_SECTION("tcp_side_channel::rx_data");
    std::string str = "CONN ID: " + std::to_string(conn_id) + "\n";
    str += " - DATA: " + std::string(data.begin(), data.end());
    CORE_LOG_INFO("Received data on TCP listener:\n{}", str);
  }

  void connection_opened(other::natural_t conn_id) override {
    CORE_LOG_INFO("TCP connection opened with conn ID {}", conn_id);
  }

  void connection_closed(other::natural_t conn_id) override {
    CORE_LOG_INFO("TCP connection closed with conn ID {}", conn_id);
  }
};

OTHER_PROVIDES(tcp_side_channel, other::packet_sink, "tcp_recorder", OTHER_PARAMS(OTHER_PARAM("transport", "tcp")))
OTHER_PLUGIN(tcp_recorder, "0.1.0", "N/A", "records TCP traffic for debugging purposes")
