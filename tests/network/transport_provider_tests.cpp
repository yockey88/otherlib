/**
 * \file transport_provider_tests.cpp
 **/
#include "transport_provider_tests.hpp"

#include "core/time.hpp"

#include "gtest/gtest.h"
#include "network_thread_test_runner.hpp"

namespace other {

  TEST_F(transport_provider_tests, verify_mock_transport_provider) {
    scope<mock_transport_provider> mock_provider = make_scope<mock_transport_provider>();
    ASSERT_NE(mock_provider, nullptr);

    // twice because hash() calls name()
    EXPECT_CALL(*mock_provider, name()).Times(2).WillRepeatedly(testing::Return("MockTransport"));
    EXPECT_CALL(*mock_provider, is_reliable()).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_provider, is_ordered()).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(*mock_provider, is_datagram()).Times(1).WillOnce(testing::Return(false));

    EXPECT_EQ(mock_provider->name(), "MockTransport");
    EXPECT_EQ(mock_provider->hash(), FNV("mocktransport"));
    EXPECT_TRUE(mock_provider->is_reliable());
    EXPECT_TRUE(mock_provider->is_ordered());
    EXPECT_FALSE(mock_provider->is_datagram());

    mock_provider = nullptr;
  }

  TEST_F(transport_provider_tests, lifecycle_test) {
    using ::testing::AtLeast;

    scope<mock_transport_provider> mock_provider = make_scope<mock_transport_provider>();
    EXPECT_CALL(*mock_provider, name()).WillRepeatedly(testing::Return("MockTransport"));
    EXPECT_CALL(*mock_provider, on_initialize()).Times(1);
    EXPECT_CALL(*mock_provider, on_tick()).Times(AtLeast(1));
    EXPECT_CALL(*mock_provider, on_begin_shutdown()).Times(1);
    EXPECT_CALL(*mock_provider, on_shutdown()).Times(1);

    network_thread_test_runner test_runner;
    EXPECT_TRUE(test_runner.test_passes("MockTransportProvider lifecycle", [&](network_thread_test_context& context) {
      context.net_thread.register_provider(mock_provider.get());
      std::this_thread::sleep_for(microseconds(100));
      return true;
    }));

    mock_provider = nullptr;
  }

  TEST_F(transport_provider_tests, lifecycle_asserts) {
    using ::testing::AtLeast;

    scope<mock_transport_provider> mock_provider = make_scope<mock_transport_provider>();
    EXPECT_CALL(*mock_provider, name()).WillRepeatedly(testing::Return("MockTransport"));
    EXPECT_CALL(*mock_provider, on_initialize()).Times(1);
    EXPECT_CALL(*mock_provider, on_tick()).Times(AtLeast(1));
    EXPECT_CALL(*mock_provider, on_begin_shutdown()).Times(1);
    EXPECT_CALL(*mock_provider, on_shutdown()).Times(1);

    message_bus bus;
    network_thread thread{ bus };
    io net_io;

    ASSERT_DEATH(mock_provider->initialize(nullptr, nullptr), ".*");
    ASSERT_DEATH(mock_provider->initialize(&thread, nullptr), ".*");
    ASSERT_DEATH(mock_provider->initialize(nullptr, &net_io), ".*");

    mock_provider->initialize(&thread, &net_io);
    ASSERT_DEATH(mock_provider->initialize(&thread, &net_io), ".*");

    mock_provider->tick();
    mock_provider->begin_shutdown();
    mock_provider->shutdown();
    ASSERT_DEATH(mock_provider->tick(), ".*");
    ASSERT_DEATH(mock_provider->begin_shutdown(), ".*");
    ASSERT_DEATH(mock_provider->shutdown(), ".*");

    mock_provider = nullptr;
  }

}  // namespace other