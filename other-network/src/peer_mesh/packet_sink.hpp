/**
 * \file peer_mesh/packet_sink.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP
#define OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP

#include "core/defines.hpp"
#include "core/interfaces.hpp"
#include "core/job_system.hpp"

namespace other {

  class OTHER_CLASS packet_sink {
    OTHER_ENVIRONMENT_INTERFACE("Network", "PacketSink", job_system*);

   public:
    packet_sink(job_system* jobs, const std::string& name)
        : jobs(jobs), name(name) {}
    virtual ~packet_sink() = default;

    void set_job_system(job_system* jobs);

    /// delivery entry points, called from the owning transport's thread. the default
    ///  implementations job-hop to the main thread; implementations that need ordered
    ///  same-thread delivery (the mesh's transport adapters) override these directly
    virtual void rx_data(natural_t conn_id, std::span<const uint8_t> data);
    virtual void connection_opened(natural_t conn_id);
    virtual void connection_closed(natural_t conn_id);

   protected:
    /// called on MAIN THREAD
    virtual void on_rx_data(natural_t conn_id, std::span<const uint8_t> data) = 0;
    virtual void on_connection_opened(natural_t conn_id) = 0;
    virtual void on_connection_closed(natural_t conn_id) = 0;

   private:
    job_system* jobs;
    const std::string name;
  };

  inline auto packet_sink_args(job_system* jobs) {
    return [jobs]() {
      return std::tuple<job_system*>{ jobs };
    };
  }

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP