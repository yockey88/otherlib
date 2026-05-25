/**
 * \file tests/vm/parsing_tests.cpp
 **/

#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/vm_tests.hpp"

namespace other {
  namespace detail {

    ::testing::AssertionResult check_small_named_code_blocks_test(const ocmd_ir& ir);

  }  // namespace detail

  TEST_F(vm_tests, oasm_simple_loose_code_block) {
    std::string source = R"(
  dumpx r1
  dumpx r2
  dumpx r3
  load r4, my_data.first_addr
  ret
    )";
    /*
    const auto ir = parse_source(source);
    ASSERT_TRUE(ir.valid);
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& r1 = get_register_case(1);
    const auto& r2 = get_register_case(2);
    const auto& r3 = get_register_case(3);
    const auto& r4 = get_register_case(4);
    const std::array expected_instructions = {
      expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x00, 0x02),
        .arguments = { make_register_argument(r1) },
      },
      expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x00, 0x02),
        .arguments = { make_register_argument(r2) },
      },
      expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x00, 0x02),
        .arguments = { make_register_argument(r3) },
      },
      expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x01, 0x01),
        .arguments = { make_register_argument(r4), make_label_argument("my_data.first_addr") },
      },
      expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x02, 0x04),
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "code_block_0", expected_instructions);
    */

    ;
    const auto tokens = ocmd_lexer{ source }.tokenize();
    EXPECT_FALSE(tokens.empty());

    ASSERT_EQ(tokens.size(), 15);
    EXPECT_EQ(tokens[0].type, TOKEN_TYPE_SOURCE_START);
    EXPECT_EQ(tokens[0].text, "");
    EXPECT_EQ(tokens[1].type, TOKEN_TYPE_KW_DUMPX);
    EXPECT_EQ(tokens[1].text, "dumpx");
    EXPECT_EQ(tokens[2].type, TOKEN_TYPE_KW_R1);
    EXPECT_EQ(tokens[2].text, "r1");
    EXPECT_EQ(tokens[3].type, TOKEN_TYPE_KW_DUMPX);
    EXPECT_EQ(tokens[3].text, "dumpx");
    EXPECT_EQ(tokens[4].type, TOKEN_TYPE_KW_R2);
    EXPECT_EQ(tokens[4].text, "r2");
    EXPECT_EQ(tokens[5].type, TOKEN_TYPE_KW_DUMPX);
    EXPECT_EQ(tokens[5].text, "dumpx");
    EXPECT_EQ(tokens[6].type, TOKEN_TYPE_KW_R3);
    EXPECT_EQ(tokens[6].text, "r3");
    EXPECT_EQ(tokens[7].type, TOKEN_TYPE_KW_LOAD);
    EXPECT_EQ(tokens[7].text, "load");
    EXPECT_EQ(tokens[8].type, TOKEN_TYPE_KW_R4);
    EXPECT_EQ(tokens[8].text, "r4");
    EXPECT_EQ(tokens[9].type, TOKEN_TYPE_COMMA);
    EXPECT_EQ(tokens[9].text, ",");
    EXPECT_EQ(tokens[10].type, TOKEN_TYPE_IDENTIFIER);
    EXPECT_EQ(tokens[10].text, "my_data");
    EXPECT_EQ(tokens[11].type, TOKEN_TYPE_DOT);
    EXPECT_EQ(tokens[11].text, ".");
    EXPECT_EQ(tokens[12].type, TOKEN_TYPE_IDENTIFIER);
    EXPECT_EQ(tokens[12].text, "first_addr");
    EXPECT_EQ(tokens[13].type, TOKEN_TYPE_KW_RET);
    EXPECT_EQ(tokens[13].text, "ret");
    EXPECT_EQ(tokens[14].type, TOKEN_TYPE_EOF);
    EXPECT_EQ(tokens[14].text, "");

    for (const auto& token : tokens) {
      CORE_LOG_DEBUG("Token [{}]: value = {} ({}:{})", token.type, token.text, token.line_number, token.column_number);
    }

    const auto ir = oasm_parser{ tokens }.parse();
    EXPECT_FALSE(ir.code_blocks.empty());
    EXPECT_TRUE(ir.data_blocks.empty());

    // single unnamed code block with 5 instructions
    EXPECT_EQ(ir.code_blocks.size(), 1);

    const auto& code_blk = ir.code_blocks[0];
    EXPECT_EQ(code_blk.name, "code_block_0");
    EXPECT_EQ(code_blk.instructions.size(), 5);

    EXPECT_EQ(code_blk.instructions[0].category_and_type, opcode_with_category_and_type(0x00, 0x02));
    ASSERT_EQ(code_blk.instructions[0].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
    EXPECT_EQ(code_blk.instructions[0].arguments[0].raw_txt, "r1");

    EXPECT_EQ(code_blk.instructions[1].category_and_type, opcode_with_category_and_type(0x00, 0x02));
    ASSERT_EQ(code_blk.instructions[1].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[1].arguments[0].type, TOKEN_TYPE_KW_R2);
    EXPECT_EQ(code_blk.instructions[1].arguments[0].raw_txt, "r2");

    EXPECT_EQ(code_blk.instructions[2].category_and_type, opcode_with_category_and_type(0x00, 0x02));
    ASSERT_EQ(code_blk.instructions[2].arguments.size(), 1);
    EXPECT_EQ(code_blk.instructions[2].arguments[0].type, TOKEN_TYPE_KW_R3);
    EXPECT_EQ(code_blk.instructions[2].arguments[0].raw_txt, "r3");

    EXPECT_EQ(code_blk.instructions[3].category_and_type, opcode_with_category_and_type(0x01, 0x01));
    ASSERT_EQ(code_blk.instructions[3].arguments.size(), 2);
    EXPECT_EQ(code_blk.instructions[3].arguments[0].type, TOKEN_TYPE_KW_R4);
    EXPECT_EQ(code_blk.instructions[3].arguments[0].raw_txt, "r4");
    EXPECT_EQ(code_blk.instructions[3].arguments[1].type, TOKEN_TYPE_LABEL);
    EXPECT_EQ(code_blk.instructions[3].arguments[1].raw_txt, "my_data.first_addr");

    EXPECT_EQ(code_blk.instructions[4].category_and_type, opcode_with_category_and_type(0x02, 0x04));
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
      load r1, 0x1234
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
      load r1, 0x1234
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
      load r1, 0x1234
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

  TEST_F(vm_tests, simple_oasm_data_block_single_object) {
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
    ASSERT_EQ(ir.data_blocks[0].objects.size(), 1);

    EXPECT_EQ(ir.data_blocks[0].objects[0].name, "first_addr");
    EXPECT_EQ(ir.data_blocks[0].objects[0].type, OCMD_DATA_TYPE_ADDRESS)
      << std::format("Expected data object type to be ADDRESS, but found {} (text: '{}')", ir.data_blocks[0].objects[0].type, ir.data_blocks[0].objects[0].value_token.text);
    EXPECT_EQ(ir.data_blocks[0].objects[0].value_token.type, TOKEN_TYPE_DATA_VALUE)
      << std::format("Expected value token type to be DATA_VALUE, but found {} (text: '{}')", ir.data_blocks[0].objects[0].value_token.type, ir.data_blocks[0].objects[0].value_token.text);
    EXPECT_EQ(ir.data_blocks[0].objects[0].value_token.text, "0x1234");
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
      EXPECT_EQ(foo_block.instructions[0].category_and_type, opcode_with_category_and_type(0x00, 0x01));
      if (foo_block.instructions[0].arguments.size() != 1) {
        return ::testing::AssertionFailure() << "Expected 1 argument for first instruction in 'foo' block, but found " << foo_block.instructions[0].arguments.size();
      }
      EXPECT_EQ(foo_block.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
      EXPECT_EQ(foo_block.instructions[0].arguments[0].raw_txt, "r1");
      EXPECT_EQ(foo_block.instructions[1].category_and_type, opcode_with_category_and_type(0x00, 0x01));
      if (foo_block.instructions[1].arguments.size() != 1) {
        return ::testing::AssertionFailure() << "Expected 1 argument for second instruction in 'foo' block, but found " << foo_block.instructions[1].arguments.size();
      }
      EXPECT_EQ(foo_block.instructions[1].arguments[0].type, TOKEN_TYPE_KW_R2);
      EXPECT_EQ(foo_block.instructions[1].arguments[0].raw_txt, "r2");
      EXPECT_EQ(foo_block.instructions[2].category_and_type, opcode_with_category_and_type(0x02, 0x04));
      if (foo_block.instructions[2].arguments.size() != 0) {
        return ::testing::AssertionFailure() << "Expected 0 arguments for third instruction in 'foo' block, but found " << foo_block.instructions[2].arguments.size();
      }

      const auto& bar_block = ir.code_blocks[1];
      EXPECT_EQ(bar_block.instructions[0].category_and_type, opcode_with_category_and_type(0x01, 0x01));
      if (bar_block.instructions[0].arguments.size() != 2) {
        return ::testing::AssertionFailure() << "Expected 2 arguments for first instruction in 'bar' block, but found " << bar_block.instructions[0].arguments.size();
      }
      EXPECT_EQ(bar_block.instructions[0].arguments[0].type, TOKEN_TYPE_KW_R1);
      EXPECT_EQ(bar_block.instructions[0].arguments[0].raw_txt, "r1");
      EXPECT_EQ(bar_block.instructions[0].arguments[1].type, TOKEN_TYPE_ADDRESS);
      EXPECT_EQ(bar_block.instructions[0].arguments[1].raw_txt, "1234");
      EXPECT_EQ(bar_block.instructions[1].category_and_type, opcode_with_category_and_type(0x02, 0x04));
      if (bar_block.instructions[1].arguments.size() != 0) {
        return ::testing::AssertionFailure() << "Expected 0 arguments for second instruction in 'bar' block, but found " << bar_block.instructions[1].arguments.size();
      }

      return ::testing::AssertionSuccess();
    }

  }  // namespace detail
}  // namespace other