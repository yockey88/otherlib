/**
 * \file message/message_serialization_tests.cpp
 **/
#include <gtest/gtest.h>

#include "message/message.hpp"

#include "other_test.hpp"

namespace other {

  /// regression shape: a trivially-copyable field AFTER a blob only deserializes if the
  ///  codec advances past the blob bytes (the historical misparse)
  struct blob_then_field_probe {
    natural_t lead = 0;
    ostd::vector<uint8_t> payload;
    natural_t tail = 0;
  };

}  // namespace other

OTHER_REFLECT(
  other::blob_then_field_probe,
  OTHER_MSG_FIELD(lead, CONNECTION_ID),
  OTHER_MSG_FIELD(payload, DATA),
  OTHER_MSG_FIELD(tail, NEW_CONNECTION_ID))

namespace other {

  class message_serialization_tests : public other_test {
   protected:
    template <typename T>
    std::pair<T, size_t> round_trip(const T& value, ostd::vector<uint8_t>* raw = nullptr) {
      ostd::vector<uint8_t> bytes = serialize_direct(value);
      if (raw != nullptr) {
        *raw = bytes;
      }
      return deserialize_direct<T>(bytes);
    }

    static void expect_bytes_equal(const ostd::vector<uint8_t>& lhs, const ostd::vector<uint8_t>& rhs) {
      ASSERT_EQ(lhs.size(), rhs.size());
      for (size_t i = 0; i < lhs.size(); ++i) {
        ASSERT_EQ(lhs[i], rhs[i]) << "byte mismatch at offset " << i;
      }
    }
  };

  TEST_F(message_serialization_tests, round_trips_every_reflected_message_struct) {
    {
      message_header value{ .category = COMMAND, .id = LISTEN_CONNECTION };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out, value);
      EXPECT_EQ(consumed, sizeof(uint16_t) * 2);
    }
    {
      binding_point value{ 0x7f000001, 49222 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.ip, value.ip);
      EXPECT_EQ(out.port, value.port);
    }
    {
      version value{ .major = 1, .minor = 2, .patch = 3 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.major, 1);
      EXPECT_EQ(out.minor, 2);
      EXPECT_EQ(out.patch, 3);
    }
    {
      acknowledgement_ack value{ .ack_id = 77, .acked_header = { COMMAND, SHUTDOWN_REQUEST }, .ack = 1 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.ack_id, 77u);
      EXPECT_EQ(out.acked_header, value.acked_header);
      EXPECT_EQ(out.ack, 1);
    }
    {
      notification_connect_connection value{
        .connection_endpoint = { 0x0A000001, 1000 },
        .endpoint = { 0x0A000002, 2000 },
        .connection_id = 5,
        .new_connection_id = 6,
        .transport_hash = 0xABCD,
      };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.connection_endpoint.ip, value.connection_endpoint.ip);
      EXPECT_EQ(out.endpoint.port, value.endpoint.port);
      EXPECT_EQ(out.connection_id, 5u);
      EXPECT_EQ(out.new_connection_id, 6u);
      EXPECT_EQ(out.transport_hash, 0xABCDu);
    }
    {
      notification_close_connection value{ .connection_id = 9, .transport_hash = 0x1234 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.connection_id, 9u);
      EXPECT_EQ(out.transport_hash, 0x1234u);
    }
    {
      command_listen_connection value{ .endpoint = { 0x7f000001, 8080 }, .connection_id = 3, .transport_hash = 42 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.endpoint.port, 8080);
      EXPECT_EQ(out.connection_id, 3u);
      EXPECT_EQ(out.transport_hash, 42u);
    }
    {
      command_connect_connection value{ .endpoint = { 0x7f000001, 8081 }, .connection_id = 4, .transport_hash = 43 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.endpoint.port, 8081);
      EXPECT_EQ(out.connection_id, 4u);
      EXPECT_EQ(out.transport_hash, 43u);
    }
    {
      command_close_connection value{ .connection_id = 8, .transport_hash = 44 };
      auto [out, consumed] = round_trip(value);
      EXPECT_EQ(out.connection_id, 8u);
      EXPECT_EQ(out.transport_hash, 44u);
    }
    {
      command_tx_data value{ .connection_id = 11, .data = { 0xDE, 0xAD, 0xBE, 0xEF } };
      ostd::vector<uint8_t> raw;
      auto [out, consumed] = round_trip(value, &raw);
      EXPECT_EQ(out.connection_id, 11u);
      expect_bytes_equal(out.data, value.data);
      EXPECT_EQ(consumed, raw.size());
    }
    {
      request_acknowledgment value{ .ack_id = 21, .original_header = { COMMAND, TX_DATA }, .message_data = { 1, 2, 3 } };
      ostd::vector<uint8_t> raw;
      auto [out, consumed] = round_trip(value, &raw);
      EXPECT_EQ(out.ack_id, 21u);
      EXPECT_EQ(out.original_header, value.original_header);
      expect_bytes_equal(out.message_data, value.message_data);
      /// consumed must cover the blob bytes, not stop at its length prefix
      EXPECT_EQ(consumed, raw.size());
    }
  }

  TEST_F(message_serialization_tests, field_after_blob_survives) {
    blob_then_field_probe value{
      .lead = 0x1111,
      .payload = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE },
      .tail = 0x2222,
    };

    ostd::vector<uint8_t> raw;
    auto [out, consumed] = round_trip(value, &raw);
    EXPECT_EQ(out.lead, 0x1111u);
    expect_bytes_equal(out.payload, value.payload);
    EXPECT_EQ(out.tail, 0x2222u);
    EXPECT_EQ(consumed, raw.size());
  }

  TEST_F(message_serialization_tests, blob_larger_than_u16_round_trips) {
    /// join snapshots routinely exceed the old u16 length cap
    constexpr size_t kBlobSize = 100'000;
    command_tx_data value{ .connection_id = 1 };
    for (size_t i = 0; i < kBlobSize; ++i) {
      value.data.push_back(static_cast<uint8_t>(i * 31 + 7));
    }

    ostd::vector<uint8_t> raw;
    auto [out, consumed] = round_trip(value, &raw);
    EXPECT_EQ(out.connection_id, 1u);
    expect_bytes_equal(out.data, value.data);
    EXPECT_EQ(consumed, raw.size());
  }

  TEST_F(message_serialization_tests, oversize_blob_length_is_an_error_not_a_misparse) {
    /// a length prefix claiming more bytes than remain must throw, never read past the end
    command_tx_data value{ .connection_id = 2, .data = { 1, 2, 3, 4 } };
    ostd::vector<uint8_t> bytes = serialize_direct(value);

    /// corrupt the u32 length prefix (sits right after the u64 connection id)
    bytes[sizeof(natural_t)] = 0xFF;
    bytes[sizeof(natural_t) + 1] = 0xFF;
    EXPECT_THROW((deserialize_direct<command_tx_data>(bytes)), buffer_parsing_error);
  }

}  // namespace other
