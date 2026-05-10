/**
 * \file other-editor/plugins/management-plugin/tcp_listener.hpp
 **/
#ifndef OTHER_EDITOR_MANAGEMENT_PLUGIN_TCP_LISTENER_HPP
#define OTHER_EDITOR_MANAGEMENT_PLUGIN_TCP_LISTENER_HPP

#include "peer-mesh/packet_sink.hpp"

class OTHER_CLASS tcp_listener : public other::packet_sink {
 public:
  tcp_listener(other::job_system& jobs)
      : packet_sink(jobs, "TcpListener") {}
  ~tcp_listener() override = default;

  void on_rx_data(other::natural_t from_peer_id, std::vector<uint8_t> data) override;
  void on_connection_opened(other::natural_t peer_id) override;
  void on_connection_closed(other::natural_t peer_id) override;
};

#endif  // OTHER_EDITOR_MANAGEMENT_PLUGIN_TCP_LISTENER_HPP