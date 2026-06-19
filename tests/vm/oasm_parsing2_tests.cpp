/**
 * \file vm/oasm_parsing2_tests.cpp
 **/

#include "vm_tests.hpp"

namespace other {

  TEST_F(vm_tests, oasm_parsing2_simple_label) {
    const std::string_view source = R"(
    /entry: main
    #data {
      .greeting : = "hello"
      .count : = 17
      .payload : = 0A 0B 0C
    }
    $main:
      dump data.greeting
      dump data.count
      dump data.payload
      ret
    end
    )";

    diagnostic_engine diag;
    const auto ir = detail::parse_source(source, &diag);
    ASSERT_TRUE(ir.valid);

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
    const std::array expected_code = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::expected_argument{ .type = TOKEN_TYPE_LABEL, .raw_txt = "data.greeting" } },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::expected_argument{ .type = TOKEN_TYPE_LABEL, .raw_txt = "data.count" } },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::expected_argument{ .type = TOKEN_TYPE_LABEL, .raw_txt = "data.payload" } },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };
    const std::array expected_definitions = {
      detail::expected_definition{
        .name = "entry",
        .value = "main",
      },
    };

    detail::expect_data_block_matches(ir.data_blocks[0], "data", expected_objects);
    detail::expect_code_block_matches(ir.code_blocks[0], "main", expected_code);
    detail::expect_definition_matches(ir.definitions[0], "entry", "main");
  }

}  // namespace other