/**
 * \file peer-mesh/packet_sink.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP
#define OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP

#include "core/defines.hpp"
#include "core/job_system.hpp"

namespace other {

  class packet_sink {
   public:
    packet_sink(job_system& jobs, const std::string& name = "UnnamedPacketSink")
        : jobs(jobs), name(name) {}
    virtual ~packet_sink() = default;

    /// any thread accept main
    void rx_data(natural_t from_peer_id, std::span<const uint8_t> data);
    void connection_opened(natural_t peer_id);
    void connection_closed(natural_t peer_id);

   protected:
    /// called on MAIN THREAD
    virtual void on_rx_data(natural_t from_peer_id, std::span<const uint8_t> data) = 0;
    virtual void on_connection_opened(natural_t peer_id) = 0;
    virtual void on_connection_closed(natural_t peer_id) = 0;

   private:
    job_system& jobs;
    const std::string name;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP