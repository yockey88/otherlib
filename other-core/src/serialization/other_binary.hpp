/**
 * \file serialization/other_binary.hpp
 **/
#ifndef OTHER_CORE_SERIALIZATION_OTHER_BINARY_HPP
#define OTHER_CORE_SERIALIZATION_OTHER_BINARY_HPP

#include "core/defines.hpp"

namespace other {

  struct other_binary_header {
    // Other Environment Binary Encoding
    uint8_t kMagic[sizeof(uint32_t)] = { 'O', 'E', 'B', 'E' };

    uint16_t version = 1;
    uint16_t flags = 0;

    natural_t root_fingerprint = 0;
    natural_t root_name_hash = 0;

    uint32_t payload_offset = 0;
    uint32_t ref_table_offset = 0;
    uint32_t blob_offset = 0;
    uint32_t schema_offset = 0;
    uint8_t reserved[22] = { 0 };
  };
  static_assert(sizeof(other_binary_header) == 64, "other_binary_header must be 64 bytes");
  static_assert(std::is_trivially_copyable_v<other_binary_header>);

}  // namespace other

#endif  // OTHER_CORE_SERIALIZATION_OTHER_BINARY_HPP