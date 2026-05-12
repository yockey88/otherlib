/**
 * \file message/message_fields.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGE_FIELDS_HPP
#define OTHER_CORE_MESSAGE_MESSAGE_FIELDS_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  enum class message_field : uint16_t {
    MESSAGE_CAT,
    MESSAGE_ID,

    PORT_NUMBER,
    IP_ADDRESS,

    MAJOR_VERSION_NUM,
    MINOR_VERSION_NUM,
    PATCH_VERSION_NUM,

    ACK_ID,
    ACKED_HEADER,
    ACK,

    CONNECTION_ENDPOINT,
    ENDPOINT,

    CONNECTION_ID,
    NEW_CONNECTION_ID,

    TRANSPORT_HASH,

    SINK_ID,

    DATA,
  };

  constexpr static std::string_view kMessageFieldNames[] = {
    "message-category",
    "message-id",

    "port-number",
    "ip-address",

    "major-version-num",
    "minor-version-num",
    "patch-version-num",

    "ack-id",
    "acked-header",
    "ack",

    "connection-endpoint",
    "endpoint",

    "connection-id",
    "new-connection-id",

    "transport-hash",

    "sink-id",

    "data",
  };

  constexpr static value_type kMessageFieldTypes[] = {
    value_type::UINT16,  // MESSAGE_CAT
    value_type::UINT16,  // MESSAGE_ID

    value_type::UINT16,  // PORT_NUMBER
    value_type::UINT32,  // IP_ADDRESS

    value_type::UINT16,  // MAJOR_VERSION_NUM
    value_type::UINT16,  // MINOR_VERSION_NUM
    value_type::UINT16,  // PATCH_VERSION_NUM

    value_type::UINT64,     // ACK_ID
    value_type::USER_TYPE,  // ACKED_HEADER
    value_type::UINT8,      // ACK

    value_type::USER_TYPE,  // CONNECTION_ENDPOINT
    value_type::USER_TYPE,  // ENDPOINT

    value_type::UINT64,  // CONNECTION_ID
    value_type::UINT64,  // NEW_CONNECTION_ID

    value_type::UINT64,  // TRANSPORT_HASH
    value_type::UINT64,  // SINK_ID

    value_type::USER_TYPE,  // DATA
  };

}  // namespace other

#endif  // OTHER_CORE_MESSAGE_MESSAGE_FIELDS_HPP