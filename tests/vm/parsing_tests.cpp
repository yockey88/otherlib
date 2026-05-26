/**
 * \file tests/vm/parsing_tests.cpp
 **/
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/vm_tests.hpp"

namespace other {
  namespace detail {

    ::testing::AssertionResult check_small_named_code_blocks_test(const ocmd_ir& ir);

  }  // namespace detail

  TEST_F(vm_tests, oasm_simple_loose_code_block) {
    std::string source = R"(
  dump r1
  dump r2
  dump r3
  set r4, my_data.first_addr
  ret
    )";
    const auto tokens = ocmd_lexer{ source }.tokenize();
    EXPECT_FALSE(tokens.empty());

    ASSERT_EQ(tokens.size(), 13);
    EXPECT_EQ(tokens[0].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[0].text, "dump");
    EXPECT_EQ(tokens[1].type, TOKEN_TYPE_KW_R1);
    EXPECT_EQ(tokens[1].text, "r1");
    EXPECT_EQ(tokens[2].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[2].text, "dump");
    EXPECT_EQ(tokens[3].type, TOKEN_TYPE_KW_R2);
    EXPECT_EQ(tokens[3].text, "r2");
    EXPECT_EQ(tokens[4].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[4].text, "dump");
    EXPECT_EQ(tokens[5].type, TOKEN_TYPE_KW_R3);
    EXPECT_EQ(tokens[5].text, "r3");
    EXPECT_EQ(tokens[6].type, TOKEN_TYPE_KW_SET);
    EXPECT_EQ(tokens[6].text, "set");
    EXPECT_EQ(tokens[7].type, TOKEN_TYPE_KW_R4);
    EXPECT_EQ(tokens[7].text, "r4");
    EXPECT_EQ(tokens[8].type, TOKEN_TYPE_COMMA);
    EXPECT_EQ(tokens[8].text, ",");
    EXPECT_EQ(tokens[9].type, TOKEN_TYPE_IDENTIFIER);
    EXPECT_EQ(tokens[9].text, "my_data");
    EXPECT_EQ(tokens[10].type, TOKEN_TYPE_DOT);
    EXPECT_EQ(tokens[10].text, ".");
    EXPECT_EQ(tokens[11].type, TOKEN_TYPE_IDENTIFIER);
    EXPECT_EQ(tokens[11].text, "first_addr");
    EXPECT_EQ(tokens[12].type, TOKEN_TYPE_KW_RET);
    EXPECT_EQ(tokens[12].text, "ret");

    for (const auto& token : tokens) {
      CORE_LOG_DEBUG("Token [{}]: value = {} ({}:{})", token.type, token.text, token.line_number, token.column_number);
    }

    const auto ir = oasm_parser{ tokens }.parse();
    EXPECT_FALSE(ir.code_blocks.empty());
    EXPECT_TRUE(ir.data_blocks.empty());

    // single unnamed code block with 5 instructions
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& code_blk = ir.code_blocks[0];
    EXPECT_EQ(code_blk.name, "code_block_0");
    ASSERT_EQ(code_blk.instructions.size(), 5);

    EXPECT_EQ(code_blk.instructions[0].opcode, canonical_opcode::DUMP_OP);
    ASSERT_EQ(code_blk.instructions[0].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
    EXPECT_EQ(code_blk.instructions[0].arguments[0].raw_txt, "r1");

    EXPECT_EQ(code_blk.instructions[1].opcode, canonical_opcode::DUMP_OP);
    ASSERT_EQ(code_blk.instructions[1].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[1].arguments[0].type, TOKEN_TYPE_KW_R2);
    EXPECT_EQ(code_blk.instructions[1].arguments[0].raw_txt, "r2");

    EXPECT_EQ(code_blk.instructions[2].opcode, canonical_opcode::DUMP_OP);
    ASSERT_EQ(code_blk.instructions[2].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[2].arguments[0].type, TOKEN_TYPE_KW_R3);
    EXPECT_EQ(code_blk.instructions[2].arguments[0].raw_txt, "r3");

    EXPECT_EQ(code_blk.instructions[3].opcode, canonical_opcode::SET_OP);
    ASSERT_EQ(code_blk.instructions[3].arguments.size(), 2);
    EXPECT_EQ(code_blk.instructions[3].arguments[0].type, TOKEN_TYPE_KW_R4);
    EXPECT_EQ(code_blk.instructions[3].arguments[0].raw_txt, "r4");
    EXPECT_EQ(code_blk.instructions[3].arguments[1].type, TOKEN_TYPE_LABEL);
    EXPECT_EQ(code_blk.instructions[3].arguments[1].raw_txt, "my_data.first_addr");

    EXPECT_EQ(code_blk.instructions[4].opcode, canonical_opcode::RET_OP);
    ASSERT_EQ(code_blk.instructions[4].arguments.size(), 0);
  }

  TEST_F(vm_tests, simple_oasm_named_code_blocks) {
    {
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    $bar:
      set r1, 0x1234
      ret
    )";

      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ tokens }.parse();
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    end
    $bar:
      set r1, 0x1234
      ret
    end
    )";

      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ tokens }.parse();
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    $bar:
      set r1, 0x1234
      ret
    end
    )";

      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ tokens }.parse();
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    end
    $bar:
      load r1, 0x1234
      ret
    )";

      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ tokens }.parse();
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }
  }

  TEST_F(vm_tests, oasm_data_block_with_tagged_data_objects) {
    std::string source = R"(
    #data {
      .first_addr : address = 0x1234
    }
    )";

    const auto tokens = ocmd_lexer{ source }.tokenize();
    EXPECT_FALSE(tokens.empty());
    const auto ir = oasm_parser{ tokens }.parse();
    ASSERT_TRUE(ir.valid);
    ASSERT_EQ(ir.data_blocks.size(), 1);
    ASSERT_EQ(ir.data_blocks[0].objects.size(), 2);

    EXPECT_EQ(ir.data_blocks[0].objects[0].name, "first_addr");
    EXPECT_EQ(ir.data_blocks[0].objects[0].type, OCMD_DATA_TYPE_ADDRESS);
    EXPECT_EQ(ir.data_blocks[0].objects[0].value_token.type, TOKEN_TYPE_HEX_LITERAL);
    EXPECT_EQ(ir.data_blocks[0].objects[0].value_token.text, "0x1234");
  }

  TEST_F(vm_tests, oasm_parser_parses_mixed_simple_data_and_code_blocks) {
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
    ASSERT_EQ(ir.data_blocks.size(), 2);
    ASSERT_EQ(ir.code_blocks.size(), 2);

    const std::array expected_first_data = {
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
    };
    const std::array expected_second_data = {
      detail::expected_data_object{
        .name = "bytes",
        .value_text = "DE AD BE EF",
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

    expect_data_block_matches(ir.data_blocks[0], "data", expected_first_data);
    expect_data_block_matches(ir.data_blocks[1], "data", expected_second_data);

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

  TEST_F(vm_tests, oasm_parser_deduces_simple_data_types_without_explicit_type_labels) {
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
        .type = OCMD_DATA_TYPE_I32,
      },
      detail::expected_data_object{
        .name = "payload",
        .value_text = "0A 0B 0C",
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

  TEST_F(vm_tests, oasm_parser_rejects_missing_data_block_closer) {
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

  TEST_F(vm_tests, oasm_parser_rejects_instruction_with_too_many_arguments) {
    const std::string_view source = R"(
    $main:
      load r1, 0x1234, r2
      ret
    )";

    const auto ir = detail::parse_source(source);
    EXPECT_FALSE(ir.valid);
    EXPECT_TRUE(ir.data_blocks.empty());
    EXPECT_TRUE(ir.code_blocks.empty());
  }

  TEST_F(vm_tests, oasm_parser_light_fuzzing_simple_code_and_data_files) {
    detail::generator gen{};

    for (size_t iteration = 0; iteration < 32; ++iteration) {
      const detail::generated_program program = detail::generate_simple_oasm_program(gen, iteration);
      SCOPED_TRACE(std::string{ "iteration=" } + std::to_string(iteration));
      SCOPED_TRACE(program.source);

      const auto ir = detail::parse_source(program.source);
      ASSERT_TRUE(ir.valid);
      ASSERT_EQ(ir.data_blocks.size(), program.data_blocks.size());
      ASSERT_EQ(ir.code_blocks.size(), program.code_blocks.size());

      for (size_t data_block_index = 0; data_block_index < program.data_blocks.size(); ++data_block_index) {
        expect_data_block_matches(ir.data_blocks[data_block_index], "data", program.data_blocks[data_block_index].objects);
      }

      for (size_t code_block_index = 0; code_block_index < program.code_blocks.size(); ++code_block_index) {
        expect_code_block_matches(
          ir.code_blocks[code_block_index],
          program.code_blocks[code_block_index].name,
          program.code_blocks[code_block_index].instructions
        );
      }
    }
  }

  namespace detail {

    ::testing::AssertionResult check_small_named_code_blocks_test(const ocmd_ir& ir) {
      if (ir.code_blocks.size() != 2) {
        return ::testing::AssertionFailure() << "Expected 2 code blocks, but found " << ir.code_blocks.size();
      }
      EXPECT_EQ(ir.code_blocks[0].name, "foo");
      EXPECT_EQ(ir.code_blocks[1].name, "bar");
      EXPECT_EQ(ir.code_blocks[0].instructions.size(), 3);
      EXPECT_EQ(ir.code_blocks[1].instructions.size(), 2);

      const auto& foo_block = ir.code_blocks[0];
      EXPECT_EQ(foo_block.instructions[0].opcode, canonical_opcode::DUMP_OP);
      if (foo_block.instructions[0].arguments.size() != 1) {
        return ::testing::AssertionFailure() << "Expected 1 argument for first instruction in 'foo' block, but found " << foo_block.instructions[0].arguments.size();
      }
      EXPECT_EQ(foo_block.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
      EXPECT_EQ(foo_block.instructions[0].arguments[0].raw_txt, "r1");
      EXPECT_EQ(foo_block.instructions[1].opcode, canonical_opcode::DUMP_OP);
      if (foo_block.instructions[1].arguments.size() != 1) {
        return ::testing::AssertionFailure() << "Expected 1 argument for second instruction in 'foo' block, but found " << foo_block.instructions[1].arguments.size();
      }
      EXPECT_EQ(foo_block.instructions[1].arguments[0].type, TOKEN_TYPE_KW_R2);
      EXPECT_EQ(foo_block.instructions[1].arguments[0].raw_txt, "r2");
      EXPECT_EQ(foo_block.instructions[2].opcode, canonical_opcode::RET_OP);
      if (foo_block.instructions[2].arguments.size() != 0) {
        return ::testing::AssertionFailure() << "Expected 0 arguments for third instruction in 'foo' block, but found " << foo_block.instructions[2].arguments.size();
      }

      const auto& bar_block = ir.code_blocks[1];
      EXPECT_EQ(bar_block.instructions[0].opcode, canonical_opcode::SET_OP);
      if (bar_block.instructions[0].arguments.size() != 2) {
        return ::testing::AssertionFailure() << "Expected 2 arguments for first instruction in 'bar' block, but found " << bar_block.instructions[0].arguments.size();
      }
      EXPECT_EQ(bar_block.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
      EXPECT_EQ(bar_block.instructions[0].arguments[0].raw_txt, "r1");
      EXPECT_EQ(bar_block.instructions[0].arguments[1].type, TOKEN_TYPE_ADDRESS);
      EXPECT_EQ(bar_block.instructions[0].arguments[1].raw_txt, "1234");
      EXPECT_EQ(bar_block.instructions[1].opcode, canonical_opcode::RET_OP);
      if (bar_block.instructions[1].arguments.size() != 0) {
        return ::testing::AssertionFailure() << "Expected 0 arguments for second instruction in 'bar' block, but found " << bar_block.instructions[1].arguments.size();
      }

      return ::testing::AssertionSuccess();
    }

  }  // namespace detail
}  // namespace other