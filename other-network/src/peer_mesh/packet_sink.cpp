/**
 * \file peer-mesg/packet_sink.cpp
 **/
#include "peer_mesh/packet_sink.hpp"
#include "data-structures/std_container.hpp"

#include "core/profiler.hpp"
#include "thread/thread_safety.hpp"

namespace other {

  void packet_sink::set_job_system(job_system* jobs) {
    this->jobs = jobs;
  }

  void packet_sink::rx_data(natural_t conn_id, std::span<const uint8_t> data) {
    PROFILE_SECTION("packet_sink::rx_data");
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
      [this, id = conn_id, d = ostd::vector<uint8_t>(data.begin(), data.end())]() {
        ASSERT_MAIN_THREAD();
        PROFILE_SECTION("packet_sink::on_rx_data");
        on_rx_data(id, d);
      });
  }

  void packet_sink::connection_opened(natural_t conn_id) {
    PROFILE_SECTION("packet_sink::connection_opened");
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
      [this, id = conn_id]() {
        ASSERT_MAIN_THREAD();
        PROFILE_SECTION("packet_sink::on_connection_opened");
        on_connection_opened(id);
      });
  }

  void packet_sink::connection_closed(natural_t conn_id) {
    PROFILE_SECTION("packet_sink::connection_closed");
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
      [this, id = conn_id]() {
        ASSERT_MAIN_THREAD();
        PROFILE_SECTION("packet_sink::on_connection_closed");
        on_connection_closed(id);
      });
  }

}  // namespace other