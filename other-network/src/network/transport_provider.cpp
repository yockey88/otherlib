/**
 * \file network/transport_provider.cpp
 **/
#include "network/transport_provider.hpp"

#include "network/network_thread.hpp"

namespace other {

  void transport_provider::initialize(network_thread* host_thread, io* net_io) {
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when initializing transport provider.");
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when initializing transport provider.");

    this->host_thread = host_thread;
    this->net_io = net_io;

    on_initialize();
  }

  void transport_provider::tick() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when ticking transport provider.");
    on_tick();
  }

  void transport_provider::begin_shutdown() {
    on_begin_shutdown();
  }

  void transport_provider::shutdown() {
    on_shutdown();

    net_io = nullptr;
    host_thread = nullptr;
  }

}  // namespace other