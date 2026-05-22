/**
 * \file peer-mesg/packet_sink.cpp
 **/
#include "peer-mesh/packet_sink.hpp"

#include "thread/thread_safety.hpp"

namespace other {

  void packet_sink::set_job_system(job_system* jobs) {
    this->jobs = jobs;
  }

  void packet_sink::rx_data(natural_t from_peer_id, std::span<const uint8_t> data) {
    if (jobs == nullptr) {
      CORE_LOG_ERROR("Packet sink '{}' received data but job system is not set. Data will be dropped.", name);
      return;
    }

    jobs->submit(
      {
        .name = "packet_sink::on_rx_data",
        .priority = job::priority::HIGH,
        .thread_affinity = job::affinity::MAIN_THREAD,
      },
      [this, id = from_peer_id, d = std::vector<uint8_t>(data.begin(), data.end())]() {
        ASSERT_MAIN_THREAD();
        on_rx_data(id, d);
      }
    );
  }

  void packet_sink::connection_opened(natural_t peer_id) {
    if (jobs == nullptr) {
      CORE_LOG_ERROR("Packet sink '{}' received connection opened event but job system is not set. Event will be ignored.", name);
      return;
    }

    jobs->submit(
      {
        .name = "packet_sink::on_connection_opened",
        .priority = job::priority::HIGH,
        .thread_affinity = job::affinity::MAIN_THREAD,
      },
      [this, id = peer_id]() {
        ASSERT_MAIN_THREAD();
        on_connection_opened(id);
      }
    );
  }

  void packet_sink::connection_closed(natural_t peer_id) {
    if (jobs == nullptr) {
      CORE_LOG_ERROR("Packet sink '{}' received connection closed event but job system is not set. Event will be ignored.", name);
      return;
    }

    jobs->submit(
      {
        .name = "packet_sink::on_connection_closed",
        .priority = job::priority::HIGH,
        .thread_affinity = job::affinity::MAIN_THREAD,
      },
      [this, id = peer_id]() {
        ASSERT_MAIN_THREAD();
        on_connection_closed(id);
      }
    );
  }

}  // namespace other