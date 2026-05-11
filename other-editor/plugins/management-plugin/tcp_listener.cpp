/**
 * \file tcp_listener.cpp
 **/
#include "tcp_listener.hpp"

void tcp_listener::on_rx_data(other::natural_t from_peer_id, std::span<const uint8_t> data) {
  std::string str = "PEER ID: " + std::to_string(from_peer_id) + "\n";
  str += " - DATA: " + std::string(data.begin(), data.end());
  CORE_LOG_INFO("Received data on TCP listener:\n{}", str);
}

void tcp_listener::on_connection_opened(other::natural_t peer_id) {
  CORE_LOG_INFO("TCP connection opened with peer ID {}", peer_id);
}

void tcp_listener::on_connection_closed(other::natural_t peer_id) {
  CORE_LOG_INFO("TCP connection closed with peer ID {}", peer_id);
}