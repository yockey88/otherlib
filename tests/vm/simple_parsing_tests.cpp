/**
 * \file tests/vm/parsing_tests.cpp
 **/

#include <string>

#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/vm_diagnostic.hpp"
#include "vm/vm_tests.hpp"

namespace other {
  namespace detail {

    ::testing::AssertionResult check_small_named_code_blocks_test(const ocmd_ir& ir);

  }  // namespace detail

  TEST_F(vm_tests, simple_oasm_parsing_loose_code_block) {
    std::string source = R"(
  dump r1
  dump r2
  dump r3
  set r4, my_data.first_addr
  ret
    )";

    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty());

    ASSERT_EQ(tokens.size(), 15);
    EXPECT_EQ(tokens[0].type, TOKEN_TYPE_SOURCE_START);
    EXPECT_EQ(tokens[0].text, "");
    EXPECT_EQ(tokens[1].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[1].text, "dump");
    EXPECT_EQ(tokens[2].type, TOKEN_TYPE_KW_R1);
    EXPECT_EQ(tokens[2].text, "r1");
    EXPECT_EQ(tokens[3].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[3].text, "dump");
    EXPECT_EQ(tokens[4].type, TOKEN_TYPE_KW_R2);
    EXPECT_EQ(tokens[4].text, "r2");
    EXPECT_EQ(tokens[5].type, TOKEN_TYPE_KW_DUMP);
    EXPECT_EQ(tokens[5].text, "dump");
    EXPECT_EQ(tokens[6].type, TOKEN_TYPE_KW_R3);
    EXPECT_EQ(tokens[6].text, "r3");
    EXPECT_EQ(tokens[7].type, TOKEN_TYPE_KW_SET);
    EXPECT_EQ(tokens[7].text, "set");
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
      CORE_LOG_DEBUG("Token [{}]: value = {} source-range([{}, {}])", token.type, token.text, token.source_view.start, token.source_view.end);
    }

    const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
    EXPECT_FALSE(ir.code_blocks.empty());
    EXPECT_TRUE(ir.data_blocks.empty());

    // single unnamed code block with 5 instructions
    EXPECT_EQ(ir.code_blocks.size(), 1);

    const auto& code_blk = ir.code_blocks[0];
    EXPECT_EQ(code_blk.name, "__natural_entry");
    EXPECT_EQ(code_blk.instructions.size(), 5);

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

  TEST_F(vm_tests, simple_oasm_parsing_named_code_blocks) {
    {
      SCOPED_TRACE("Test with missing 'end' for both code blocks");
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    $bar:
      set r1, 0x1234
      ret
    )";

      diagnostic_engine diag;
      const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      SCOPED_TRACE("Test with 'end' only for first code block");
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

      diagnostic_engine diag;
      const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      SCOPED_TRACE("Test with 'end' only for second code block");
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

      diagnostic_engine diag;
      const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }

    {
      SCOPED_TRACE("Test with missing 'end' for first code block");
      std::string source = R"(
    $foo:
      dump r1
      dump r2
      ret
    end
    $bar:
      set r1, 0x1234
      ret
    )";

      diagnostic_engine diag;
      const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
      EXPECT_FALSE(tokens.empty());
      const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      EXPECT_TRUE(detail::check_small_named_code_blocks_test(ir));
    }
  }

  TEST_F(vm_tests, simple_oasm_parsingdata_block_single_object) {
    std::string source = R"(
    #data {
      .first_addr : address = 0x1234
    }
    )";

    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty());
    const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
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

  TEST_F(vm_tests, simple_oasm_parsing_write_and_set) {
    std::string source = R"(
#data_1 {
  .data_1_1 : string = "my string"
}
$code_0:
  write r2, data_1_1
  ret
end
$code_0_1:
  set r5, 0x53CF
  write r5, data_1_1
  write r1, data_1_1
  ret
  )";

    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty());
    const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_EQ(ir.data_blocks.size(), 1);
    ASSERT_EQ(ir.code_blocks.size(), 2);

    const std::array expected_objects = {
      detail::expected_data_object{
        .name = "data_1_1",
        .value_text = "my string",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string("my string"),
      },
    };

    const std::array expected_code_0 = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R2,
            .raw_txt = "r2",
            .value = 0x02,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_LABEL,
            .raw_txt = "data_1_1",
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    const std::array expected_code_0_1 = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R5,
            .raw_txt = "r5",
            .value = 0x05,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_ADDRESS,
            .raw_txt = "53CF",
            .value = 0x53CF,
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R5,
            .raw_txt = "r5",
            .value = 0x05,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_LABEL,
            .raw_txt = "data_1_1",
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R1,
            .raw_txt = "r1",
            .value = 0x01,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_LABEL,
            .raw_txt = "data_1_1",
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_data_block_matches(ir.data_blocks[0], "data_1", expected_objects);
    expect_code_block_matches(ir.code_blocks[0], "code_0", expected_code_0);
    expect_code_block_matches(ir.code_blocks[1], "code_0_1", expected_code_0_1);
  }

  TEST_F(vm_tests, simple_mov_instruction_parsing) {
    std::string source = R"(
    $main:
      set r1, 10
      set r2, 20
      mov r2, r3
      dump r1
      dump r2
      dump r3
      ret
    )";

    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty());
    const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_TRUE(ir.data_blocks.empty());
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const auto& r1 = detail::get_register_case(1);
    const auto& r2 = detail::get_register_case(2);
    const auto& r3 = detail::get_register_case(3);
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
        .expected_opcode = canonical_opcode::MOV_OP,
        .arguments = { detail::make_register_argument(r2), detail::make_register_argument(r3) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r1) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r2) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r3) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
  }

  TEST_F(vm_tests, simple_oasm_parsing_fuzz_failure_1) {
    std::string source = R"(
#data_10_0 {
  .data_10_0_0 : address = 0x49CD
  .data_10_0_1 : float = 10.5
  .data_10_0_2 : string = "str_10_0_2"
}
$code_10_0:
  set r7, 0x9787
  dump rd
  ret
/def_10_1_0:value_10_1_0
#data_10_1 {
  .data_10_1_0 : int32 = 36
  .data_10_1_1 : address = 0xF43B
  .data_10_1_2 : string = "str_10_1_2"
}
$code_10_1:
  write r5, data_10_1_0
  write ra, data_10_0_0
  ret
$code_10_2:
  dump rf
  ret
  )";

    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty());
    const auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
    ASSERT_TRUE(ir.valid);
    ASSERT_EQ(ir.data_blocks.size(), 2);
    ASSERT_EQ(ir.code_blocks.size(), 3);

    const std::array expected_first_data = {
      detail::expected_data_object{
        .name = "data_10_0_0",
        .value_text = "0x49CD",
        .type = OCMD_DATA_TYPE_ADDRESS,
        .data = detail::raw_bytes_from_value<uint16_t>(0x49CD),
      },
      detail::expected_data_object{
        .name = "data_10_0_1",
        .value_text = "10.5",
        .type = OCMD_DATA_TYPE_F32,
        .data = detail::raw_bytes_from_value<float>(10.5f),
      },
      detail::expected_data_object{
        .name = "data_10_0_2",
        .value_text = "str_10_0_2",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string("str_10_0_2"),
      },
    };

    const std::array expected_second_data = {
      detail::expected_data_object{
        .name = "data_10_1_0",
        .value_text = "36",
        .type = OCMD_DATA_TYPE_I32,
        .data = detail::raw_bytes_from_value<int32_t>(36),
      },
      detail::expected_data_object{
        .name = "data_10_1_1",
        .value_text = "0xF43B",
        .type = OCMD_DATA_TYPE_ADDRESS,
        .data = detail::raw_bytes_from_value<uint16_t>(0xF43B),
      },
      detail::expected_data_object{
        .name = "data_10_1_2",
        .value_text = "str_10_1_2",
        .type = OCMD_DATA_TYPE_STRING,
        .data = detail::raw_bytes_from_string("str_10_1_2"),
      },
    };

    const std::array expected_code_10_0 = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R7,
            .raw_txt = "r7",
            .value = vm_register_idx::VM_R7,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_ADDRESS,
            .raw_txt = "9787",
            .value = 0x9787,
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_RD,
            .raw_txt = "rd",
            .value = 0x0D,  // assuming rd maps to register number 13 (0x0D)
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    const std::array expected_code_10_1 = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_R5,
            .raw_txt = "r5",
            .value = vm_register_idx::VM_R5,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_LABEL,
            .raw_txt = "data_10_1_0",
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_RA,
            .raw_txt = "ra",
            .value = vm_register_idx::VM_RA,
          },
          detail::expected_argument{
            .type = TOKEN_TYPE_LABEL,
            .raw_txt = "data_10_0_0",
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    const std::array expected_code_10_2 = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = {
          detail::expected_argument{
            .type = TOKEN_TYPE_KW_RF,
            .raw_txt = "rf",
            .value = vm_register_idx::VM_RF,
          },
        },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_data_block_matches(ir.data_blocks[0], "data_10_0", expected_first_data);
    expect_data_block_matches(ir.data_blocks[1], "data_10_1", expected_second_data);
    expect_code_block_matches(ir.code_blocks[0], "code_10_0", expected_code_10_0);
    expect_code_block_matches(ir.code_blocks[1], "code_10_1", expected_code_10_1);
    expect_code_block_matches(ir.code_blocks[2], "code_10_2", expected_code_10_2);
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