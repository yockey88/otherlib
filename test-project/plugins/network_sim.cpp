/**
 * \file plugins/network_sim.cpp
 **/
#include "network/transport_provider.hpp"

#include "plugin/plugin.hpp"

class OTHER_API network_simulator : public other::transport_provider {
 public:
  virtual ~network_simulator() = default;

  std::string name() const override { return "Network Simulator"; }

 private:
  void tx_data(other::natural_t connection_id, ostd::vector<uint8_t>&& data) override {}
  void close(other::natural_t connection_id) override {}

  void on_initialize() override {}
  void on_tick() override {}
  void on_shutdown() override {}

  void on_start_listen(other::natural_t conn_id, const other::binding_point& endpoint) override {}
  void on_start_connect(other::natural_t conn_id, const other::binding_point& endpoint) override {}
};

OTHER_PROVIDES(network_simulator, other::transport_provider, "network_simulator");
OTHER_PLUGIN(net_sim, "0.1.0", "N/A", "N/A")