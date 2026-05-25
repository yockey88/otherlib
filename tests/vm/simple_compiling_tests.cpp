/**
 * \file vm/simple_compiling_tests.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include "vm_tests.hpp"

namespace other {

  TEST_F(vm_tests, oasm_parsing_simple_label) {
    const std::string_view source = R"(
    $main:
      ret
    end
    )";

    const auto ir = detail::parse_source(source);
    ASSERT_TRUE(ir.valid);
    ASSERT_TRUE(ir.data_blocks.empty());
    ASSERT_EQ(ir.code_blocks.size(), 1);

    const std::array expected_code = {
      detail::expected_instruction{
        .category_and_type = opcode_with_category_and_type(0x02, 0x04),
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);

    const auto bytecode = ocmd_compiler{ ir }.compile();
  }

}  // namespace other