/**
 * \file vm/other_device.cpp
 **/
#include "vm/other_device.hpp"

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "vm/command_bus.hpp"
#include "vm/command_files/ocmd_headers.hpp"
#include "vm/decompiler.hpp"

namespace other {

  uint16_t other_command_device::program_metadata::full_program_size() const {
    return data_offset + data_size;
  }

  uint16_t other_command_device::program_metadata::get_global_data_address() const {
    return load_address + data_offset;
  }

  uint16_t other_command_device::program_metadata::get_global_entry_point_address() const {
    return load_address + entry_point_offset;
  }

  uint16_t other_command_device::program_metadata::get_instruction_address_by_index(uint16_t instruction_index) const {
    return load_address + (instruction_index * other_command_device::kOpCodeSize);
  }

  uint16_t other_command_device::program_metadata::get_data_address_by_offset(uint16_t data_offset) const {
    return load_address + (data_offset - sizeof(ocmd_file_header));
  }

  uint16_t other_command_device::globalize_address(uint16_t local_address) const {
    return globalize_address(&current_program_metadata, local_address);
  }

  uint16_t other_command_device::globalize_address(const program_metadata* md, uint16_t local_address) const {
    OTHER_ASSERT(md != nullptr, "Program metadata pointer cannot be null");
    return md->load_address + local_address;
  }

  const void* other_command_device::access_current_program_memory(uint16_t program_offset) {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return nullptr;
    }
    // clang-format on

    uint16_t absolute_address = globalize_address(&current_program_metadata, program_offset);
    return memory->unsafe_at(absolute_address);
  }

  const void* other_command_device::access_current_program_data_section(uint16_t program_offset) {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return nullptr;
    }
    // clang-format on

    uint16_t absolute_address = current_program_metadata.get_data_address_by_offset(program_offset);
    return memory->unsafe_at(absolute_address);
  }

  natural_t other_command_device::current_program_data_as_u64(uint16_t data_offset) const {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return 0;
    }
    // clang-format on

    uint16_t absolute_address = current_program_metadata.get_data_address_by_offset(data_offset);
    return read_u64_at(absolute_address);
  }

  void other_command_device::write_current_program_memory(uint16_t address, const uint8_t* bytes, size_t length) {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return;
    }
    // clang-format on

    uint16_t absolute_address = globalize_address(&current_program_metadata, address);
    memory->unsafe_write(bytes, length, absolute_address);
  }

  void other_command_device::write_current_program_data(uint16_t address, const uint8_t* bytes, size_t length) {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return;
    }
    // clang-format on

    uint16_t absolute_address = current_program_metadata.get_data_address_by_offset(address);
    memory->unsafe_write(bytes, length, absolute_address);
  }

  void other_command_device::write_current_program_data_from_u64(uint16_t data_offset, uint64_t value) {
    // clang-format off
    if ((current_program_metadata.state & program_state::PROGRAM_STOPPED)) {
      return;
    }
    // clang-format on

    uint16_t absolute_address = current_program_metadata.get_data_address_by_offset(data_offset);
    write_u64_at(absolute_address, value);
  }

  uint8_t other_command_device::get_random_byte() {
    return static_cast<uint8_t>(rng.next());
  }

  void other_command_device::write_u64_at(const size_t address, const uint64_t value) {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    const void* value_ptr = &value;
    memory->unsafe_write(static_cast<const uint8_t*>(value_ptr), sizeof(uint64_t), address);
  }

  uint64_t other_command_device::read_u64_at(const size_t address) const {
    assert(address + sizeof(uint64_t) <= kMemorySize && "Address out of bounds");
    const void* value_ptr = memory->unsafe_at(address);
    return *static_cast<const uint64_t*>(value_ptr);
  }

}  // namespace other