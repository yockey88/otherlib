/**
 * \file vm/vm_tests.cpp
 **/
#include "vm/vm_tests.hpp"

#include <gtest/gtest.h>

#include "vm/code_generator_000.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/command_files/ocmd_linker.hpp"
#include "vm/control_table.hpp"
#include "vm/default_symbol_resolver.hpp"
#include "vm/vm.hpp"

namespace other {
  namespace detail {

    std::vector<uint8_t> compile_and_link_program(const std::string_view source);
    std::vector<uint8_t> compile_test_program1();

  }  // namespace detail

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

  TEST_F(vm_tests, vm_program1) {
    const std::vector<uint8_t> bytes = detail::compile_test_program1();
    ASSERT_FALSE(bytes.empty());

    other_command_device device;
    ASSERT_NO_FATAL_FAILURE(vm::initialize_device(&device));

    ASSERT_NE(device.memory, nullptr);
    ASSERT_NO_FATAL_FAILURE(vm::activate_builtin_control_table(&device, OTHER_CONTROL_TABLE_V000));
    ASSERT_NE(device.control_table, nullptr);

    ASSERT_NO_FATAL_FAILURE(vm::load_bytes_to_address(&device, device.program_load_cursor, bytes.data(), bytes.size()));

    // do {
    //   vm::execute_next_instruction(&device);
    // } while (!device.halted);

    ASSERT_NO_FATAL_FAILURE(vm::shutdown_device(&device));
  }

  namespace detail {

    std::vector<uint8_t> compile_and_link_program(const std::string_view source) {
      auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty())
        << std::format("Tokenization failed for source:\n{}", source);
      if (tokens.empty()) {
        return {};
      }

      auto ir = oasm_parser{ vm_version{}, tokens }.parse();
      EXPECT_TRUE(ir.valid)
        << std::format("Parsing failed for source:\n{}", source);
      if (!ir.valid) {
        return {};
      }

      auto program = ocmd_compiler{ ir }.compile(make_scope<code_generator_000>());
      EXPECT_TRUE(program.valid)
        << std::format("Compilation failed for source:\n{}", source);
      if (!program.valid) {
        return {};
      }

      auto resolver = make_scope<default_symbol_resolver>();
      std::vector<uint8_t> bytes = ocmd_linker{ program }.link(std::move(resolver));
      return bytes;
    }

    std::vector<uint8_t> compile_test_program1() {
      const std::string_view source = R"(
      #data {
        .number : int32 = 42
      }
      $main:
        set r1, data.number
        dump r1
        ret
      end
      )";

      return compile_and_link_program(source);
    }

  }  // namespace detail
}  // namespace other