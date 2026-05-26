/**
 * \file vm/oasm_parsing_tests.cpp
 **/
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/code_generator_000.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"

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

    generated_program generate_simple_oasm_program(detail::generator& gen, const size_t iteration) {
      constexpr size_t int_type_choice = 0;
      constexpr size_t string_type_choice = 1;
      constexpr size_t blob_type_choice = 2;
      constexpr size_t float_type_choice = 3;
      constexpr size_t address_type_choice = 4;

      generated_program program;
      std::vector<std::string> available_labels;

      const size_t data_block_count = gen.data_block_count_dist(gen.gen);
      for (size_t data_block_index = 0; data_block_index < data_block_count; ++data_block_index) {
        generated_data_block data_block_case;

        const bool should_gen_definitions = gen.should_define_dist(gen.gen) == 1;
        if (should_gen_definitions) {
          size_t definition_count = gen.definition_dist(gen.gen);
          for (size_t def_index = 0; def_index < definition_count; ++def_index) {
            const std::string def_name = "def_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(def_index);
            const std::string def_value = "value_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(def_index);
            program.definitions.push_back(generated_definition{
              .name = def_name,
              .value = def_value,
            });
            data_block_case.source += std::format("/{}:{}\n", def_name, def_value);
          }
        }

        data_block_case.source += std::format("#data_{}_{} {{\n", iteration, data_block_index);
        const size_t object_count = gen.object_count_dist(gen.gen);
        for (size_t object_index = 0; object_index < object_count; ++object_index) {
          const std::string object_name = "data_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(object_index);
          available_labels.push_back(object_name);

          switch (gen.data_kind_dist(gen.gen)) {
            case int_type_choice: {
              const int32_t integer_value = gen.integer_dist(gen.gen);
              data_block_case.source += "  ." + object_name + " : int32 = " + std::to_string(integer_value) + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = std::to_string(integer_value),
                .type = OCMD_DATA_TYPE_I32,
                .data = raw_bytes_from_value<int32_t>(static_cast<int32_t>(integer_value)),
              });
            } break;

            case string_type_choice: {
              const std::string string_value = "str_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(object_index);
              data_block_case.source += "  ." + object_name + " : string = \"" + string_value + "\"\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = string_value,
                .type = OCMD_DATA_TYPE_STRING,
                .data = raw_bytes_from_string(string_value),
              });
            } break;

            case blob_type_choice: {
              const size_t blob_size = gen.blob_size_dist(gen.gen);
              std::string blob_text;
              std::vector<uint8_t> blob_data;
              for (size_t byte_index = 0; byte_index < blob_size; ++byte_index) {
                const uint8_t byte = static_cast<uint8_t>(gen.byte_dist(gen.gen));
                if (!blob_text.empty()) {
                  blob_text += " ";
                }
                blob_text += hex_byte_string(byte);
                blob_data.push_back(byte);
              }

              data_block_case.source += "  ." + object_name + " : blob = " + blob_text + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = blob_text | std::views::split(' ') | std::views::join | std::ranges::to<std::string>(),
                .type = OCMD_DATA_TYPE_BLOB,
                .data = blob_data,
              });
            } break;

            case float_type_choice: {
              const float float_value = static_cast<float>(gen.integer_dist(gen.gen)) / 8.0f;
              std::string float_text = std::to_string(float_value);
              while (float_text.size() > 2 && float_text.back() == '0') {
                float_text.pop_back();
              }
              if (!float_text.empty() && float_text.back() == '.') {
                float_text.push_back('0');
              }

              data_block_case.source += "  ." + object_name + " : float = " + float_text + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = float_text,
                .type = OCMD_DATA_TYPE_F32,
                .data = raw_bytes_from_value<float>(std::stof(float_text)),
              });
            } break;

            case address_type_choice: {
              const uint16_t address_value = gen.address_dist(gen.gen);
              data_block_case.source += "  ." + object_name + " : address = 0x" + hex_word_string(address_value) + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = "0x" + hex_word_string(address_value),
                .type = OCMD_DATA_TYPE_ADDRESS,
                .data = raw_bytes_from_value<uint16_t>(address_value),
              });
            } break;

            default: {
              throw std::logic_error("Invalid data kind choice");
            } break;
          }
        }

        data_block_case.source += "}\n";
        program.data_blocks.push_back(data_block_case);
      }

      const size_t code_block_count = gen.code_block_count_dist(gen.gen);
      std::uniform_int_distribution<size_t> label_dist(0, available_labels.size() - 1);

      for (size_t code_block_index = 0; code_block_index < code_block_count; ++code_block_index) {
        generated_code_block code_block_case;
        code_block_case.name = "code_" + std::to_string(iteration) + "_" + std::to_string(code_block_index);
        code_block_case.source += "$" + code_block_case.name + ":\n";

        const size_t instruction_count = gen.instruction_count_dist(gen.gen);
        for (size_t instruction_index = 0; instruction_index < instruction_count; ++instruction_index) {
          const auto& reg = get_register_case(gen.register_dist(gen.gen));
          switch (gen.instruction_kind_dist(gen.gen)) {
            case 0: {
              code_block_case.source += "  dump " + std::string{ reg.text } + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::DUMP_OP,
                .arguments = { make_register_argument(reg) },
              });
            } break;

            case 1: {
              const std::string& label = available_labels[gen.label_dist(gen.gen) % available_labels.size()];
              code_block_case.source += "  write " + std::string{ reg.text } + ", " + label + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::WRITE_OP,
                .arguments = { make_register_argument(reg), make_label_argument(label) },
              });
            } break;

            case 2: {
              const uint16_t address = gen.address_dist(gen.gen);
              code_block_case.source += "  set " + std::string{ reg.text } + ", 0x" + hex_word_string(address) + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_address_argument(address) },
              });
            } break;

            case 3: {
              const std::string& label = available_labels[gen.label_dist(gen.gen) % available_labels.size()];
              code_block_case.source += "  set " + std::string{ reg.text } + ", " + label + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_label_argument(label) },
              });
            } break;

            default: {
              throw std::logic_error("Invalid instruction kind choice");
            } break;
          }
        }

        code_block_case.source += "  ret\n";
        code_block_case.instructions.push_back(expected_instruction{
          .expected_opcode = canonical_opcode::RET_OP,
        });

        if (gen.include_end_dist(gen.gen) == 1) {
          code_block_case.source += "end\n";
        }

        program.code_blocks.push_back(code_block_case);
      }

      const size_t max_sections = std::max(program.data_blocks.size(), program.code_blocks.size());
      for (size_t index = 0; index < max_sections; ++index) {
        if (index < program.data_blocks.size()) {
          program.source += program.data_blocks[index].source;
        }
        if (index < program.code_blocks.size()) {
          program.source += program.code_blocks[index].source;
        }
      }

      return program;
    }

    generated_program generated_simple_bad_oasm_program(generator& gen, const size_t iteration) {
      constexpr size_t missing_data_close_before_code = 0;
      constexpr size_t missing_data_close_before_directive = 1;
      constexpr size_t wrong_load_arity = 2;
      constexpr size_t invalid_address_literal = 3;
      constexpr size_t duplicate_data_object_name = 4;

      generated_program program;

      const auto& reg_a = get_register_case(gen.register_dist(gen.gen));
      const auto& reg_b = get_register_case(gen.register_dist(gen.gen));
      const std::string code_name = std::format("bad_code_{}", iteration);
      const std::string object_name = std::format("bad_obj_{}", iteration);
      const std::string duplicate_name = std::format("dup_obj_{}", iteration);
      const uint16_t first_value = gen.integer_dist(gen.gen);
      const uint16_t second_value = gen.integer_dist(gen.gen);
      const uint16_t first_address = gen.address_dist(gen.gen);
      const uint16_t second_address = gen.address_dist(gen.gen);

      switch (iteration % 5) {
        case missing_data_close_before_code: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "${}:\n"
            "  dump {}\n"
            "  ret\n",
            object_name, first_value, code_name, reg_a.text
          );
          program.expected_error_substring = "Forgot to end data block, expected '}' before code block";
        } break;

        case missing_data_close_before_directive: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : string = \"broken_{}\"\n"
            "#data {{\n"
            "  .{}_next : int32 = {}\n"
            "}}\n"
            "${}:\n"
            "  ret\n",
            object_name, iteration, object_name, first_value, code_name
          );
          program.expected_error_substring = "Forgot to end data block, expected '}' before directive block";
        } break;

        case wrong_load_arity: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : address = 0x{}\n"
            "}}\n"
            "${}:\n"
            "  dump {}\n"
            "  load {}, 0x{}, {}\n"
            "  ret\n",
            object_name, hex_word_string(first_address), code_name, reg_a.text, reg_b.text, hex_word_string(second_address), reg_a.text
          );
          program.expected_error_substring = "Expected 2 parameters for instruction 'load', but found 3";
        } break;

        case invalid_address_literal: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : address = 0x{} 0x{}\n"
            "}}\n"
            "${}:\n"
            "  ret\n",
            object_name, hex_word_string(first_address), hex_word_string(second_address), code_name
          );
          program.expected_error_substring = "Expected a single hexadecimal literal for address type, but found 2 tokens";
        } break;

        case duplicate_data_object_name: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "}}\n"
            "${}:\n"
            "  dump {}\n"
            "  ret\n"
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "}}\n",
            duplicate_name, first_value, code_name, reg_a.text, duplicate_name, second_value
          );
          program.expected_error_substring = std::format("Duplicate data object name '{}' in data block 'data'", duplicate_name);
        } break;

        default: {
          throw std::logic_error("Invalid negative test case choice");
        } break;
      }

      return program;
    }

  }  // namespace detail
}  // namespace other