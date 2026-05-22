/**
 * \file plugins/tcp_side_channel.hpp
 **/
#include "driver/driver.hpp"
#include "plugin/plugin.hpp"

#include "peer-mesh/packet_sink.hpp"

using other::packet_sink;

class OTHER_CLASS tcp_side_channel : public other::packet_sink {
 public:
  tcp_side_channel(other::job_system* jobs)
      : packet_sink(jobs, "TcpSideChannel") {}
  ~tcp_side_channel() override = default;

  void on_rx_data(other::natural_t from_peer_id, std::span<const uint8_t> data) override {
    std::string str = "PEER ID: " + std::to_string(from_peer_id) + "\n";
    str += " - DATA: " + std::string(data.begin(), data.end());
    CORE_LOG_INFO("Received data on TCP listener:\n{}", str);
  }

  void on_connection_opened(other::natural_t peer_id) override {
    CORE_LOG_INFO("TCP connection opened with peer ID {}", peer_id);
  }

  void on_connection_closed(other::natural_t peer_id) override {
    CORE_LOG_INFO("TCP connection closed with peer ID {}", peer_id);
  }
};

OTHER_PROVIDES(tcp_side_channel, other::packet_sink, "tcp_recorder", OTHER_PARAMS(OTHER_PARAM("transport", "tcp")))
OTHER_PLUGIN(tcp_recorder, "0.0.1", "N/A", "records TCP traffic for debugging purposes")