/**
 * \file vm/simple_compiling_tests.cpp
 **/
#include "vm/code_generator_000.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/opcode.hpp"

#include "vm_tests.hpp"

namespace other {

  TEST_F(vm_tests, ocmd_simple_compile_main_fn) {
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
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    expect_code_block_matches(ir.code_blocks[0], "main", expected_code);

    ocmd_compiler compiler{ ir };
    const auto bytecode = compiler.compile(make_scope<code_generator_000>());
    ASSERT_EQ(bytecode.compiled_blocks.size(), 1);
    EXPECT_EQ(bytecode.compiled_blocks[0].name, "main");
    // EXPECT_TRUE(bytecode[0].is_entry_point);

    const auto& bc = bytecode.compiled_blocks[0].artifact.machine_instructions;
    ASSERT_EQ(bc.size(), 1);
    EXPECT_EQ(bc[0].opcode, opcode_return());
    CORE_LOG_DEBUG("Encoded bytecode for main function:");
    for (const auto& instr : bc) {
      CORE_LOG_DEBUG("  {:#010x}", instr.opcode);
    }
  }

  TEST_F(vm_tests, ocmd_simple_compile_main_fn2) {
    const std::string_view source = R"(
    /entry: main
    $main:
      set r1, 42
      set r2, 99
      dump r1
      dump r2
      ret
    end
    )";

    const auto program = detail::compile_source(source);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.definitions.size(), 1);
    ASSERT_EQ(program.compiled_blocks.size(), 1);

    detail::expect_definition_matches(program.definitions[0], "entry", "main");

    const std::array expected_main = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_direct(vm_register_idx::VM_R1, 42) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_direct(vm_register_idx::VM_R2, 99) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R1) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R2) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_return() },
    };

    detail::expect_compiled_code_block_matches(program.compiled_blocks[0], "main", false, expected_main);
  }

  TEST_F(vm_tests, ocmd_simple_compile_test_asm1) {
    const std::string source = R"(
/entry: foo

$entry:
  dump r1
  dump r2
  dump r3
  set r4, data.first_addr
  ret
end

$foo:
  set r1, 10
  set r2, 11
  set r3, data.first_addr
  call entry
  dump r4
  stopdev
end

#data {
  .first_addr := 1111
  ; .hello_str : string = "Hello, Other Command Device!"
  ; .player_hp := 100
  ; .my_data := DE AD BE EF FA CE BE EF ; blob type
  ; .my_second_data : blob = 0x010203 ; also blob type
}
  )";

    const auto program = detail::compile_source(source);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.definitions.size(), 1);
    ASSERT_EQ(program.compiled_blocks.size(), 2);
    ASSERT_EQ(program.compiled_data_sections.size(), 1);

    const std::array expected_entry = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R1) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R2) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R3) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_from(vm_register_idx::VM_R4, 0xFFFF) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_return() },
    };
    const std::array expected_foo = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_direct(vm_register_idx::VM_R1, 10) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_direct(vm_register_idx::VM_R2, 11) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_from(vm_register_idx::VM_R3, 0xFFFF) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_call_at(0xFFFF) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R4) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_stop_device() },
    };

    detail::expect_definition_matches(program.definitions[0], "entry", "foo");
    detail::expect_compiled_code_block_matches(program.compiled_blocks[0], "entry", false, expected_entry);
    detail::expect_compiled_code_block_matches(program.compiled_blocks[1], "foo", false, expected_foo);
  }

}  // namespace other