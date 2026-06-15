/**
 * \file vm/oasm_parsing_tests.cpp
 **/
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/code_generator_000.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/ocmd_trace_sink.hpp"

#include "fuzzing.hpp"
#include "vm_tests.hpp"

namespace other {

  namespace detail {

    std::string read_file_suffix(const std::filesystem::path& file_path, const uintmax_t start_offset);

  }  // namespace detail

  TEST_F(vm_tests, oasm_parsing_mixed_simple_data_and_code_blocks) {
    const std::string_view source = R"(
    #data {
      .first_addr : address = 0x1234
      .banner : string = "hello"
    }
    $boot:
      set r1, first_addr
      ret
    #data {
      .bytes : blob = DE AD BE EF
      .ratio : float = 3.25
    }
    $copy:
      dump r2
      set r3, 0x4567
      ret
    end
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_TRUE(ir.valid);
    // parser recognizes they have the same name and merges them into a single data block
    ASSERT_EQ(ir.data_blocks.size(), 1);
    ASSERT_EQ(ir.code_blocks.size(), 2);

    const std::array expected_data = {
      detail::expected_data_object{
        .name = "first_addr",
        .value_text = "0x1234",
        .type = OCMD_DATA_TYPE_ADDRESS,
        .data = detail::raw_bytes_from_value<uint16_t>(0x1234),
      },
      detail::expected_data_object{
        .name = "banner",
        .value_text = "hello",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string("hello"),
      },
      detail::expected_data_object{
        .name = "bytes",
        .value_text = "DEADBEEF",
        .type = OCMD_DATA_TYPE_BLOB,
        .data = std::vector<uint8_t>{ 0xDE, 0xAD, 0xBE, 0xEF },
      },
      detail::expected_data_object{
        .name = "ratio",
        .value_text = "3.25",
        .type = OCMD_DATA_TYPE_F32,
        .data = detail::raw_bytes_from_value<float>(3.25f),
      },
    };

    expect_data_block_matches(ir.data_blocks[0], "data", expected_data);

    const auto& r1 = detail::get_register_case(1);
    const auto& r2 = detail::get_register_case(2);
    const auto& r3 = detail::get_register_case(3);
    const std::array expected_boot = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_label_argument("first_addr") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    const std::array expected_copy = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r2) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r3), detail::make_address_argument(0x4567) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "boot", expected_boot);
    expect_code_block_matches(ir.code_blocks[1], "copy", expected_copy);
  }

  TEST_F(vm_tests, oasm_parsing_deduces_simple_data_types) {
    const std::string_view source = R"(
    #data {
      .greeting : = "hello"
      .count : = 17
      .payload : = 0A 0B 0C
      .load-path : string = "C:\Program Files\Example\file.txt"
    }
    $main:
      ret
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_EQ(ir.data_blocks.size(), 1);
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const std::array expected_objects = {
      detail::expected_data_object{
        .name = "greeting",
        .value_text = "hello",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string("hello"),
      },
      detail::expected_data_object{
        .name = "count",
        .value_text = "17",
        .type = OCMD_DATA_TYPE_I64,
      },
      detail::expected_data_object{
        .name = "payload",
        .value_text = "0A0B0C",
        .type = OCMD_DATA_TYPE_BLOB,
        .data = std::vector<uint8_t>{ 0x0A, 0x0B, 0x0C },
      },
      detail::expected_data_object{
        .name = "load-path",
        .value_text = R"(C:\Program Files\Example\file.txt)",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string(R"(C:\Program Files\Example\file.txt)"),
      },
    };

    expect_data_block_matches(ir.data_blocks[0], "data", expected_objects);

    const std::array expected_code = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
  }

  TEST_F(vm_tests, oasm_parsing_rejects_missing_data_block_closer) {
    const std::string_view source = R"(
    #data {
      .answer : int32 = 42
    $main:
      ret
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    EXPECT_FALSE(ir.valid);
    EXPECT_TRUE(ir.data_blocks.empty());
    EXPECT_TRUE(ir.code_blocks.empty());
  }

  TEST_F(vm_tests, oasm_parsing_rejects_instruction_with_too_many_arguments) {
    const std::string_view source = R"(
    $main:
      set r1, 0x1234, r2
      ret
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    EXPECT_FALSE(ir.valid);
    EXPECT_TRUE(ir.data_blocks.empty());
    EXPECT_TRUE(ir.code_blocks.empty());
  }

  TEST_F(vm_tests, oasm_parsing_conjoins_three_segment_data_access_parameter) {
    const std::string_view source = R"(
    $main:
      set r1, data.player.health
      ret
    end
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_TRUE(ir.data_blocks.empty());
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& r1 = detail::get_register_case(1);
    const std::array expected_code = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_label_argument("data.player.health") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
  }

  TEST_F(vm_tests, oasm_parsing_parse_instruction_parameters_with_brackets) {
    const std::string_view source = R"(
    $main:
      set r1, [data.player.health]
      ret
    end
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_TRUE(ir.data_blocks.empty());
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& r1 = detail::get_register_case(1);
    const std::array expected_code = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_label_argument("data.player.health") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
  }

  TEST_F(vm_tests, oasm_parsing_accepts_jump_labels) {
    const std::string_view source = R"(
    $main:
/;
if 10 > 20 {
  print(30)
} else {
  print(10)
}
return;
;/
      set r1, 10
      set r2, 20
      set r3, 30
      cmp r1, r2, rflag
      jne elseblock
        dump r3
        goto endif
    @elseblock:
        dump r1
    
    @endif:
      ret
    end
    )";

    ocmd_trace_sink ts;
    diagnostic_engine diag;
    natural_t ts_id = 0;
    ASSERT_NO_FATAL_FAILURE(ts_id = diag.register_sink("tracer", &ts));
    EXPECT_NE(ts_id, 0) << "Failed to register trace sink for diagnostics";
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_NO_FATAL_FAILURE(diag.remove_sink(ts_id));
    ASSERT_TRUE(ir.valid);
    ASSERT_TRUE(ir.data_blocks.empty());
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& r1 = detail::get_register_case(1);
    const auto& r2 = detail::get_register_case(2);
    const auto& r3 = detail::get_register_case(3);
    const auto& rflag = detail::get_register_case(vm_register_idx::VM_RFLAG);
    const std::array expected_code = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_integer_literal_argument(10) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r2), detail::make_integer_literal_argument(20) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r3), detail::make_integer_literal_argument(30) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::CMP_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_register_argument(r2), detail::make_register_argument(rflag) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::JNE_OP,
        .arguments = { detail::make_label_argument("elseblock") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r3) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::GOTO_OP,
        .arguments = { detail::make_label_argument("endif") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r1) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
  }

  TEST_F(vm_tests, oasm_parsing_light_fuzzing) {
    detail::generator gen{};

    for (size_t iteration = 0; iteration < 128; ++iteration) {
      const detail::generated_program program = detail::generate_simple_oasm_program(gen, iteration);
      SCOPED_TRACE(std::format("iteration={}\n{}", iteration, program.source));

      diagnostic_engine diag;
      const auto ir = detail::parse_source(program.source, &diag);
      ASSERT_TRUE(ir.valid);
      ASSERT_EQ(ir.data_blocks.size(), program.data_blocks.size());
      ASSERT_EQ(ir.code_blocks.size(), program.code_blocks.size());

      for (size_t code_block_index = 0; code_block_index < program.code_blocks.size(); ++code_block_index) {
        expect_code_block_matches(ir.code_blocks[code_block_index], program.code_blocks[code_block_index].name, program.code_blocks[code_block_index].instructions);
      }
      for (size_t data_block_index = 0; data_block_index < program.data_blocks.size(); ++data_block_index) {
        expect_data_block_matches(ir.data_blocks[data_block_index], std::format("data_{}_{}", iteration, data_block_index), program.data_blocks[data_block_index].objects);
      }
    }
  }

  TEST_F(vm_tests, oasm_parsing_light_negative_fuzzing) {
    detail::generator gen{};

    ASSERT_NE(other_test::environment, nullptr);
    const std::filesystem::path log_path = std::filesystem::absolute(other_test::environment->config.core_log_file);

    for (size_t iteration = 0; iteration < 32; ++iteration) {
      const detail::generated_program program = detail::generated_simple_bad_oasm_program(gen, iteration);
      SCOPED_TRACE(std::format("iteration={}, Expected parser rejection: {}\n{}", iteration, program.expected_error_substring, program.source));

      diagnostic_engine diag;

      ocmd_trace_sink ts;
      natural_t ts_id = diag.register_sink("tracer", &ts);

      const auto tokens = ocmd_lexer{ program.source }.tokenize(&diag);
      ASSERT_FALSE(tokens.empty())
        << std::format("Negative fuzz input should reach the parser, but lexing failed for source:\n{}", program.source);

      // std::error_code ec;
      // uintmax_t log_size_before = 0;
      // if (std::filesystem::exists(log_path, ec)) {
      //   log_size_before = std::filesystem::file_size(log_path, ec);
      //   if (ec) {
      //     log_size_before = 0;
      //     ec.clear();
      //   }
      // }
      // ::testing::internal::CaptureStdout();
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      // std::string parser_output = ::testing::internal::GetCapturedStdout();
      // parser_output += detail::read_file_suffix(log_path, log_size_before);

      EXPECT_FALSE(ir.valid);
      EXPECT_TRUE(ir.code_blocks.empty());
      EXPECT_TRUE(ir.data_blocks.empty());
      EXPECT_TRUE(ir.definitions.empty());
      /// \todo:
      // EXPECT_NE(parser_output.find(program.expected_error_substring), std::string::npos)
      //   << std::format(
      //        "Parser rejected the program, but not for the expected reason.\nExpected substring: '{}'\nCaptured output:\n{}",
      //        program.expected_error_substring, parser_output
      //      );
    }
  }

}  // namespace other