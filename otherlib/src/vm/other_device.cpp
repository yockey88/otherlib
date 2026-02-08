/**
 * \file vm/other_device.cpp
 **/
#include "vm/other_device.hpp"

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "vm/decompiler.hpp"

namespace other {

  uint8_t other_command_device::get_random_byte() {
    return static_cast<uint8_t>(rng.next());
  }

  void other_command_device::write_u64_at(const size_t address, const uint64_t value) {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    *reinterpret_cast<uint64_t*>(memory->unsafe_at(address)) = value;
  }

  uint64_t other_command_device::read_u64_at(const size_t address) {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    return *reinterpret_cast<uint64_t*>(memory->unsafe_at(address));
  }

}  // namespace other