/**
 * \file vm/vm_tests.cpp
 **/
#include "vm/vm_tests.hpp"

#include "vm/control_table.hpp"
#include "vm/vm.hpp"

#include "gtest/gtest.h"

namespace other {

  TEST_F(vm_tests, basic_initialization_and_shutdown) {
    other_command_device device;

    ASSERT_NO_FATAL_FAILURE(vm::initialize_device(&device));
    ASSERT_NE(device.memory, nullptr);

    ASSERT_NO_FATAL_FAILURE(vm::activate_builtin_control_table(&device, OTHER_CONTROL_TABLE_V000));
    ASSERT_NE(device.control_table, nullptr);

    const std::vector<uint8_t> bytes = {
      0x00, 0x01, 0x02, 0x03,
      0x10, 0x11, 0x12, 0x13,
      0x20, 0x21, 0x22, 0x23,
      0x30, 0x31, 0x32, 0x33
    };
    ASSERT_NO_FATAL_FAILURE(vm::load_bytes_to_address(&device, device.program_load_cursor, bytes.data(), bytes.size()));
    uint32_t idx = 0;
    for (const auto b : std::span(device.memory->data + device.program_load_cursor, bytes.size())) {
      EXPECT_EQ(b, bytes[idx]) << std::format("Bytes at {} are not equal: {} != {}", idx, b, bytes[idx]);
      idx++;
    }

    ASSERT_NO_FATAL_FAILURE(vm::shutdown_device(&device));
  }

}  // namespace other