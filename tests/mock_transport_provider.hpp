/**
 * \file tests/network/mock_transport_provider.hpp
 **/
#ifndef OTHER_TESTS_NETWORK_MOCK_TRANSPORT_PROVIDER_HPP
#define OTHER_TESTS_NETWORK_MOCK_TRANSPORT_PROVIDER_HPP

#include <span>
#include <string>

#include <gmock/gmock.h>

#include "core/defines.hpp"

#include "network/transport_provider.hpp"

namespace other {

  class packet_sink;
  struct binding_point;

  class mock_transport_provider : public transport_provider {
   public:
    mock_transport_provider() {
      using ::testing::Invoke;
      using ::testing::Return;

      // Default name; tests can override with EXPECT_CALL or ON_CALL.
      ON_CALL(*this, name())
        .WillByDefault(Return(std::string{ "MockTransport" }));

      // Capability defaults match the base class (true, true, false).
      // T1.17 relies on this; T1.18 overrides per-test.
      ON_CALL(*this, is_reliable()).WillByDefault(Return(true));
      ON_CALL(*this, is_ordered()).WillByDefault(Return(true));
      ON_CALL(*this, is_datagram()).WillByDefault(Return(false));
    }

    ~mock_transport_provider() override = default;

    MOCK_METHOD(std::string, name, (), (const, override));
    MOCK_METHOD(void, tx_data, (natural_t connection_id, ostd::vector<uint8_t>&& data), (override));
    MOCK_METHOD(void, close, (natural_t connection_id), (override));

    MOCK_METHOD(void, on_registered_packet_sink, (packet_sink * sink), (override));
    MOCK_METHOD(void, on_unregistered_packet_sink, (packet_sink * sink), (override));
    MOCK_METHOD(void, connection_removed, (natural_t connection_id), (override));
    MOCK_METHOD(bool, is_reliable, (), (const, override));
    MOCK_METHOD(bool, is_ordered, (), (const, override));
    MOCK_METHOD(bool, is_datagram, (), (const, override));

    MOCK_METHOD(void, on_initialize, (), (override));
    MOCK_METHOD(void, on_tick, (), (override));
    MOCK_METHOD(void, on_shutdown, (), (override));
    MOCK_METHOD(void, on_start_listen, (natural_t conn_id, const binding_point& endpoint), (override));
    MOCK_METHOD(void, on_start_connect, (natural_t conn_id, const binding_point& endpoint), (override));

    MOCK_METHOD(void, on_begin_shutdown, (), (override));
    MOCK_METHOD(void, on_rx_data, (natural_t connection_id, std::span<const uint8_t> data), (override));
    MOCK_METHOD(void, on_connection_accepted, (natural_t listener_id, natural_t conn_id, const binding_point& remote), (override));
    MOCK_METHOD(void, on_connection_established, (natural_t conn_id, const binding_point& remote), (override));
    MOCK_METHOD(void, on_connection_socket_closed, (natural_t connection_id, connection_close_reason reason), (override));
  };

}  // namespace other

#endif  // OTHER_TESTS_NETWORK_MOCK_TRANSPORT_PROVIDER_HPP