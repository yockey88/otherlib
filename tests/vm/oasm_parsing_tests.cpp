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

    const auto ir = detail::parse_source(source);
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
    }
    $main:
      ret
    )";

    const auto ir = detail::parse_source(source);
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

    const auto ir = detail::parse_source(source);
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

    const auto ir = detail::parse_source(source);
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

    const auto ir = detail::parse_source(source);
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

  TEST_F(vm_tests, oasm_parsing_light_fuzzing) {
    detail::generator gen{};

    for (size_t iteration = 0; iteration < 128; ++iteration) {
      const detail::generated_program program = detail::generate_simple_oasm_program(gen, iteration);
      SCOPED_TRACE(std::format("iteration={}\n{}", iteration, program.source));

      const auto ir = detail::parse_source(program.source);
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

      const auto tokens = ocmd_lexer{ program.source }.tokenize();
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
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse();
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

  namespace detail {

    std::vector<uint8_t> raw_bytes_from_string(const std::string_view text) {
      return std::vector<uint8_t>(text.begin(), text.end());
    }

    std::string hex_byte_string(const uint8_t value) {
      constexpr char k_hex_digits[] = "0123456789ABCDEF";

      std::string result(2, '0');
      result[0] = k_hex_digits[(value >> 4) & 0x0F];
      result[1] = k_hex_digits[value & 0x0F];
      return result;
    }

    std::string hex_word_string(const uint16_t value) {
      constexpr char k_hex_digits[] = "0123456789ABCDEF";

      std::string result(4, '0');
      result[0] = k_hex_digits[(value >> 12) & 0x0F];
      result[1] = k_hex_digits[(value >> 8) & 0x0F];
      result[2] = k_hex_digits[(value >> 4) & 0x0F];
      result[3] = k_hex_digits[value & 0x0F];
      return result;
    }

    std::string read_file_suffix(const std::filesystem::path& file_path, const uintmax_t start_offset) {
      std::error_code ec;
      if (!std::filesystem::exists(file_path, ec) || ec) {
        return {};
      }

      const uintmax_t file_size = std::filesystem::file_size(file_path, ec);
      if (ec) {
        return {};
      }

      std::ifstream input(file_path, std::ios::binary);
      if (!input.is_open()) {
        return {};
      }

      const auto safe_offset = static_cast<std::streamoff>(std::min(start_offset, file_size));
      input.seekg(safe_offset);
      return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>{});
    }

    ocmd_ir parse_source(const std::string_view source) {
      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty())
        << std::format("Tokenization failed for source:\n{}", source);
      return oasm_parser{ vm_version{}, tokens }.parse();
    }

    ocmd_program compile_source(const std::string_view source) {
      auto ir = parse_source(source);
      if (!ir.valid) {
        return {};
      }

      return ocmd_compiler{ ir }.compile(make_scope<code_generator_000>());
    }

    void expect_argument_matches(const raw_instruction::argument& actual, const expected_argument& expected) {
      EXPECT_EQ(actual.type, expected.type)
        << std::format("Argument type mismatch. Expected: {}, Actual: {}", expected.type, actual.type);
      EXPECT_EQ(actual.raw_txt, expected.raw_txt);
      if (expected.value.has_value()) {
        ASSERT_TRUE(actual.value.has_value());
        EXPECT_EQ(actual.value.value(), expected.value.value())
          << std::format("Argument value mismatch. Expected: {}, Actual: {}", expected.value.value(), actual.value.value());
      }
      if (!expected.raw_data.empty()) {
        EXPECT_EQ(actual.raw_data, expected.raw_data)
          << std::format("Argument raw data mismatch. Expected size: {}, Actual size: {}", expected.raw_data.size(), actual.raw_data.size());
      }
    }

    void expect_instruction_matches(const raw_instruction& actual, const expected_instruction& expected) {
      EXPECT_EQ(actual.opcode, expected.expected_opcode)
        << std::format("Instruction opcode mismatch. Expected: {}, Actual: {}", expected.expected_opcode, actual.opcode);
      ASSERT_EQ(actual.arguments.size(), expected.arguments.size());

      for (size_t index = 0; index < expected.arguments.size(); ++index) {
        std::string actual_instruction_str = std::format("Opcode: {}, Arguments: [", actual.opcode);
        std::string expected_instruction_str = std::format("Opcode: {}, Arguments: [", expected.expected_opcode);
        for (const auto& arg : actual.arguments) {
          actual_instruction_str += std::format("{{type: {}, raw_txt: '{}', value: {}}}", arg.type, arg.raw_txt, arg.value.has_value() ? std::to_string(arg.value.value()) : "nullopt");
        }
        for (const auto& arg : expected.arguments) {
          expected_instruction_str += std::format("{{type: {}, raw_txt: '{}', value: {}}}", arg.type, arg.raw_txt, arg.value.has_value() ? std::to_string(arg.value.value()) : "nullopt");
        }
        actual_instruction_str += "]";
        expected_instruction_str += "]";
        SCOPED_TRACE(std::format("actual instruction argument [{}]:\n{}", index, actual_instruction_str));
        SCOPED_TRACE(std::format("expected instruction argument [{}]:\n{}", index, expected_instruction_str));
        expect_argument_matches(actual.arguments[index], expected.arguments[index]);
      }
    }

    void expect_code_block_matches(const code_block& actual, const std::string_view expected_name, const std::span<const expected_instruction> expected_instructions) {
      EXPECT_EQ(actual.name, expected_name);
      ASSERT_EQ(actual.instructions.size(), expected_instructions.size());
      for (size_t index = 0; index < expected_instructions.size(); ++index) {
        expect_instruction_matches(actual.instructions[index], expected_instructions[index]);
      }
    }

    void expect_data_block_matches(const data_block& actual, const std::string_view expected_name, const std::span<const expected_data_object> expected_objects) {
      EXPECT_EQ(actual.name, expected_name);
      ASSERT_EQ(actual.objects.size(), expected_objects.size());

      for (size_t index = 0; index < expected_objects.size(); ++index) {
        EXPECT_EQ(actual.objects[index].name, expected_objects[index].name)
          << std::format("Data object name mismatch at index {}. Expected: '{}', Actual: '{}'", index, expected_objects[index].name, actual.objects[index].name);
        EXPECT_EQ(actual.objects[index].type, expected_objects[index].type)
          // clang-format off
          << std::format("Data object type mismatch for object '{}'. Expected: {}, Actual: {} (name: {}/{})", 
                          expected_objects[index].name, expected_objects[index].type, actual.objects[index].type,
                          actual.objects[index].name, expected_objects[index].name);
        // clang-format on
        EXPECT_EQ(actual.objects[index].value_token.text, expected_objects[index].value_text);
        if (!expected_objects[index].data.empty()) {
          EXPECT_EQ(actual.objects[index].data, expected_objects[index].data)
            << std::format("Data content mismatch for object '{}', data size: {}/{}.", expected_objects[index].name, actual.objects[index].data.size(), expected_objects[index].data.size());
        }
      }
    }

    void expect_definition_matches(const compiler_definition& actual, const std::string_view expected_name, const std::string_view expected_value) {
      EXPECT_EQ(actual.name, expected_name);
      EXPECT_EQ(actual.value.text, expected_value);
    }

    void expect_machine_instruction_matches(const instruction& actual, const expected_machine_instruction& expected) {
      EXPECT_EQ(actual.opcode, expected.expected_opcode)
        << std::format("Machine instruction opcode mismatch. Expected: {:#010x}, Actual: {:#010x}", expected.expected_opcode, actual.opcode);
    }

    void expect_compiled_code_block_matches(
      const compiled_code_block& actual, const std::string_view expected_name,
      const bool expected_is_entry_point, const std::span<const expected_machine_instruction> expected_instructions
    ) {
      EXPECT_EQ(actual.name, expected_name);
      EXPECT_EQ(actual.is_entry_point, expected_is_entry_point);

      const auto& actual_instructions = actual.artifact.machine_instructions;
      std::string actual_instructions_str = "Instructions:\n";
      std::string expected_instructions_str = "Expected Instructions:\n";
      for (const auto& instr : actual_instructions) {
        actual_instructions_str += std::format("  Opcode: {:#010x}\n", instr.opcode);
      }
      for (const auto& instr : expected_instructions) {
        expected_instructions_str += std::format("  Opcode: {:#010x}\n", instr.expected_opcode);
      }
      SCOPED_TRACE(std::format("Actual compiled code block '{}':\n{}", actual.name, actual_instructions_str));
      SCOPED_TRACE(std::format("Expected compiled code block '{}':\n{}", expected_name, expected_instructions_str));
      ASSERT_EQ(actual_instructions.size(), expected_instructions.size());
      for (size_t index = 0; index < expected_instructions.size(); ++index) {
        SCOPED_TRACE(std::format("Comparing instruction at index {}: actual opcode {:#010x}, expected opcode {:#010x}", index, actual_instructions[index].opcode, expected_instructions[index].expected_opcode));
        expect_machine_instruction_matches(actual_instructions[index], expected_instructions[index]);
      }
    }

    const register_case& get_register_case(const size_t index) {
      return k_register_cases[index % k_register_cases.size()];
    }

    expected_argument make_register_argument(const register_case& reg) {
      return expected_argument{
        .type = reg.type,
        .raw_txt = std::string{ reg.text },
        .value = reg.value,
      };
    }

    expected_argument make_address_argument(const uint16_t value) {
      return expected_argument{
        .type = TOKEN_TYPE_ADDRESS,
        .raw_txt = hex_word_string(value),
        .value = value,
      };
    }

    expected_argument make_label_argument(const std::string_view label) {
      return expected_argument{
        .type = TOKEN_TYPE_LABEL,
        .raw_txt = std::string{ label },
        .value = static_cast<uint16_t>(0xFFFF),
      };
    }

    std::vector<expected_instruction> expected_foo_code_block() {
      const auto& r1 = get_register_case(1);
      const auto& r2 = get_register_case(2);

      return {
        expected_instruction{
          .expected_opcode = canonical_opcode::DUMP_OP,
          .arguments = { make_register_argument(r1) },
        },
        expected_instruction{
          .expected_opcode = canonical_opcode::DUMP_OP,
          .arguments = { make_register_argument(r2) },
        },
        expected_instruction{
          .expected_opcode = canonical_opcode::RET_OP,
        },
      };
    }

  }  // namespace detail
}  // namespace other