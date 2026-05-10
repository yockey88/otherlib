/**
 * \file tcp_listener.cpp
 **/
#include "tcp_listener.hpp"

void tcp_listener::on_rx_data(other::natural_t from_peer_id, std::vector<uint8_t> data) {
}

void tcp_listener::on_connection_opened(other::natural_t peer_id) {
}

void tcp_listener::on_connection_closed(other::natural_t peer_id) {
}