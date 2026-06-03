/**
 * \file vm/ocmd_compiler_tests.cpp
 **/
#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/code_generator_000.hpp"
#include "vm/command_files/compiler_error.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/ocmd_errors.hpp"

#include "fuzzing.hpp"
#include "vm_tests.hpp"

namespace other {
  namespace detail {

    struct expected_symbol_fixup {
      std::string symbol_name = "";
      size_t opcode_index = 0;
    };

    void expect_compiled_data_section_matches(const compiled_data_section& actual, const std::string_view expected_name, const std::span<const expected_data_object> expected_objects) {
      EXPECT_EQ(actual.name, expected_name);
      ASSERT_EQ(actual.fields.size(), expected_objects.size());

      std::vector<uint8_t> expected_data;
      size_t current_offset = 0;
      for (size_t index = 0; index < expected_objects.size(); ++index) {
        const auto& expected = expected_objects[index];
        EXPECT_EQ(actual.fields[index].name, expected.name);
        EXPECT_GE(actual.fields[index].offset, static_cast<uint32_t>(current_offset));
        EXPECT_GE(actual.fields[index].size, static_cast<uint32_t>(expected.data.size()));

        size_t size = expected.data.size();
        expected_data.insert(expected_data.end(), expected.data.begin(), expected.data.end());
        if (size % 8 != 0) {
          size_t padding_needed = 8 - (size % 8);
          for (size_t i = 0; i < padding_needed; i++) {
            expected_data.push_back(0x00);
          }
        }
        current_offset += expected.data.size();
      }

      if (actual.data.size() < expected_data.size()) {
        ADD_FAILURE() << std::format("Actual data size {} is smaller than expected data size {} for data section '{}'", actual.data.size(), expected_data.size(), actual.name);
        return;
      }

      // expected padding pattern is to pad to 8 bytes with zeros

      EXPECT_EQ(actual.data, expected_data) << std::format("Data content mismatch for data section '{}', data size: {}/{}.", actual.name, actual.data.size(), expected_data.size());
    }

    void expect_symbol_fixups_match(const lowering_artifact& actual, const std::span<const expected_symbol_fixup> expected_fixups) {
      ASSERT_EQ(actual.unresolved_labels.size(), expected_fixups.size());

      for (size_t index = 0; index < expected_fixups.size(); ++index) {
        EXPECT_EQ(actual.unresolved_labels[index].symbol_name, expected_fixups[index].symbol_name);
        EXPECT_EQ(actual.unresolved_labels[index].opcode_index, expected_fixups[index].opcode_index);
      }
    }

    std::vector<expected_machine_instruction> expected_machine_instructions_for_current_generator(
      const std::span<const detail::expected_instruction> expected_instructions) {
      std::vector<expected_machine_instruction> machine_instructions;
      machine_instructions.reserve(expected_instructions.size());

      for (const auto& expected : expected_instructions) {
        switch (expected.expected_opcode) {
          case canonical_opcode::DUMP_OP: {
            if (expected.arguments.empty()) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_dump_registers() });
            } else if (expected.arguments.size() == 1 && is_register_argument(expected.arguments[0])) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(register_value(expected.arguments[0])) });
            } else if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && expected.arguments[1].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_dump_memory_at(register_value(expected.arguments[0]), scalar_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled dump instruction shape for current generator";
            }
          } break;

          case canonical_opcode::WRITE_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && expected.arguments[1].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_write_x_to_memory(register_value(expected.arguments[0]), scalar_value(expected.arguments[1])),
              });
            } else if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_label_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_write_x_to_memory(register_value(expected.arguments[0]), 0xFFFF),
              });
            } else {
              ADD_FAILURE() << "Unhandled write instruction shape for current generator";
            }
          } break;

          case canonical_opcode::SET_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && expected.arguments[1].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_load_x_from(register_value(expected.arguments[0]), scalar_value(expected.arguments[1])),
              });
            } else if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && expected.arguments[1].type == TOKEN_TYPE_INTEGER_LITERAL) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_load_x_direct(register_value(expected.arguments[0]), scalar_value(expected.arguments[1])),
              });
            } else if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_label_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_load_x_from(register_value(expected.arguments[0]), 0xFFFF),
              });
            } else {
              ADD_FAILURE() << "Unhandled set instruction shape for current generator";
            }
          } break;

          case canonical_opcode::GOTO_OP: {
            if (expected.arguments.size() == 1 && expected.arguments[0].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_goto(scalar_value(expected.arguments[0])) });
            } else {
              ADD_FAILURE() << "Unhandled goto instruction shape for current generator";
            }
          } break;

          case canonical_opcode::JE_OP: {
            if (expected.arguments.size() == 1 && expected.arguments[0].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_jump_if_zero(scalar_value(expected.arguments[0])) });
            } else {
              ADD_FAILURE() << "Unhandled je instruction shape for current generator";
            }
          } break;

          case canonical_opcode::JNE_OP: {
            if (expected.arguments.size() == 1 && expected.arguments[0].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_jump_if_not_zero(scalar_value(expected.arguments[0])) });
            } else {
              ADD_FAILURE() << "Unhandled jne instruction shape for current generator";
            }
          } break;

          case canonical_opcode::CALL_OP: {
            if (expected.arguments.size() == 1 && expected.arguments[0].type == TOKEN_TYPE_ADDRESS) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_call_at(scalar_value(expected.arguments[0])) });
            } else {
              ADD_FAILURE() << "Unhandled call instruction shape for current generator";
            }
          } break;

          case canonical_opcode::RET_OP: {
            machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_return() });
          } break;

          case canonical_opcode::SYSCALL_OP: {
            if (expected.arguments.size() == 1 && expected.arguments[0].type == TOKEN_TYPE_INTEGER_LITERAL) {
              machine_instructions.push_back(expected_machine_instruction{ .expected_opcode = opcode_syscall(scalar_value(expected.arguments[0])) });
            } else {
              ADD_FAILURE() << "Unhandled syscall instruction shape for current generator";
            }
          } break;

          case canonical_opcode::ADD_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_register_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_add_x_y_to_x(register_value(expected.arguments[0]), register_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled add instruction shape for current generator";
            }
          } break;

          case canonical_opcode::SUB_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_register_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_sub_x_y_to_x(register_value(expected.arguments[0]), register_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled sub instruction shape for current generator";
            }
          } break;

          case canonical_opcode::MUL_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_register_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_mul_x_y_to_x(register_value(expected.arguments[0]), register_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled mul instruction shape for current generator";
            }
          } break;

          case canonical_opcode::DIV_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_register_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_div_x_y_to_x(register_value(expected.arguments[0]), register_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled div instruction shape for current generator";
            }
          } break;

          case canonical_opcode::MOD_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_register_argument(expected.arguments[1])) {
              machine_instructions.push_back(expected_machine_instruction{
                .expected_opcode = opcode_mod_x_y_to_x(register_value(expected.arguments[0]), register_value(expected.arguments[1])),
              });
            } else {
              ADD_FAILURE() << "Unhandled mod instruction shape for current generator";
            }
          } break;

          default: {
            ADD_FAILURE() << "Unhandled canonical opcode in current generator expectation helper";
          } break;
        }
      }

      return machine_instructions;
    }

    std::vector<expected_symbol_fixup> expected_symbol_fixups_for_current_generator(
      const std::span<const detail::expected_instruction> expected_instructions) {
      std::vector<expected_symbol_fixup> fixups;
      fixups.reserve(expected_instructions.size());

      size_t opcode_index = 0;
      for (const auto& expected : expected_instructions) {
        switch (expected.expected_opcode) {
          case canonical_opcode::WRITE_OP:
          case canonical_opcode::SET_OP: {
            if (expected.arguments.size() == 2 && is_register_argument(expected.arguments[0]) && is_label_argument(expected.arguments[1])) {
              fixups.push_back(expected_symbol_fixup{
                .symbol_name = expected.arguments[1].raw_txt,
                .opcode_index = opcode_index,
              });
            }
          } break;

          default:
            break;
        }

        ++opcode_index;
      }

      return fixups;
    }

  }  // namespace detail

  TEST_F(vm_tests, ocmd_compiler_current_generator_encodes_dump_forms) {
    const std::string_view source = R"(
    $foo:
      dump r1
      ret
    end
    $bar:
      dump r2, 0x1234
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 2);

    const std::array expected_foo = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_register_x(vm_register_idx::VM_R1) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_return() },
    };
    const std::array expected_bar = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_dump_memory_at(vm_register_idx::VM_R2, 0x1234) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_return() },
    };

    detail::expect_compiled_code_block_matches(program.compiled_blocks[0], "foo", false, expected_foo);
    detail::expect_compiled_code_block_matches(program.compiled_blocks[1], "bar", false, expected_bar);
  }

  TEST_F(vm_tests, ocmd_compiler_current_generator_skips_integer_literal_sets) {
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

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
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

    const auto& bc = program.compiled_blocks[0].artifact.machine_instructions;
    for (const auto& instr : bc) {
      CORE_LOG_DEBUG("  {:#010x}", instr.opcode);
    }
  }

  TEST_F(vm_tests, ocmd_compiler_data_section_and_label) {
    const std::string_view source = R"(
    #data {
      .answer : int32 = 42
    }
    $main:
      set r1, data.answer
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_data_sections.size(), 1);
    ASSERT_EQ(program.compiled_blocks.size(), 1);

    const std::array expected_main = {
      detail::expected_machine_instruction{ .expected_opcode = opcode_load_x_from(vm_register_idx::VM_R1, 0xFFFF) },
      detail::expected_machine_instruction{ .expected_opcode = opcode_return() },
    };

    detail::expect_compiled_code_block_matches(program.compiled_blocks[0], "main", false, expected_main);
  }

  TEST_F(vm_tests, ocmd_compiler_mixed_blocks) {
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
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_data_sections.size(), 1);
    ASSERT_EQ(program.compiled_blocks.size(), 2);

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

    detail::expect_compiled_data_section_matches(program.compiled_data_sections[0], "data", expected_data);
    detail::expect_compiled_code_block_matches(
      program.compiled_blocks[0], "boot", false, detail::expected_machine_instructions_for_current_generator(expected_boot));
    detail::expect_compiled_code_block_matches(
      program.compiled_blocks[1], "copy", false, detail::expected_machine_instructions_for_current_generator(expected_copy));
    detail::expect_symbol_fixups_match(
      program.compiled_blocks[0].artifact, detail::expected_symbol_fixups_for_current_generator(expected_boot));
    detail::expect_symbol_fixups_match(
      program.compiled_blocks[1].artifact, detail::expected_symbol_fixups_for_current_generator(expected_copy));
  }

  TEST_F(vm_tests, ocmd_compiler_all_supported_instructions) {
    const std::string_view source = R"(
    #data {
      .answer : int32 = 42
    }
    $main:
      dump
      dump r1
      dump r2, 0x1234
      write r3, 0x4567
      write r4, data.answer
      set r5, 0x6789
      set r6, 77
      set r7, data.answer
      goto 0x1000
      je 0x1001
      jne 0x1002
      call 0x1003
      add r1, r2
      sub r3, r4
      mul r5, r6
      div r7, r8
      mod r9, ra
      syscall 9
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 1);

    const auto& r1 = detail::get_register_case(1);
    const auto& r2 = detail::get_register_case(2);
    const auto& r3 = detail::get_register_case(3);
    const auto& r4 = detail::get_register_case(4);
    const auto& r5 = detail::get_register_case(5);
    const auto& r6 = detail::get_register_case(6);
    const auto& r7 = detail::get_register_case(7);
    const auto& r8 = detail::get_register_case(8);
    const auto& r9 = detail::get_register_case(9);
    const auto& ra = detail::get_register_case(10);
    const std::array expected_main = {
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r1) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DUMP_OP,
        .arguments = { detail::make_register_argument(r2), detail::make_address_argument(0x1234) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = { detail::make_register_argument(r3), detail::make_address_argument(0x4567) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::WRITE_OP,
        .arguments = { detail::make_register_argument(r4), detail::make_label_argument("data.answer") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r5), detail::make_address_argument(0x6789) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r6), detail::expected_argument{ .type = TOKEN_TYPE_INTEGER_LITERAL, .raw_txt = "77", .value = 77 } },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SET_OP,
        .arguments = { detail::make_register_argument(r7), detail::make_label_argument("data.answer") },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::GOTO_OP,
        .arguments = { detail::make_address_argument(0x1000) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::JE_OP,
        .arguments = { detail::make_address_argument(0x1001) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::JNE_OP,
        .arguments = { detail::make_address_argument(0x1002) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::CALL_OP,
        .arguments = { detail::make_address_argument(0x1003) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::ADD_OP,
        .arguments = { detail::make_register_argument(r1), detail::make_register_argument(r2) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SUB_OP,
        .arguments = { detail::make_register_argument(r3), detail::make_register_argument(r4) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::MUL_OP,
        .arguments = { detail::make_register_argument(r5), detail::make_register_argument(r6) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::DIV_OP,
        .arguments = { detail::make_register_argument(r7), detail::make_register_argument(r8) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::MOD_OP,
        .arguments = { detail::make_register_argument(r9), detail::make_register_argument(ra) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::SYSCALL_OP,
        .arguments = { detail::make_integer_literal_argument(9) },
      },
      detail::expected_instruction{
        .expected_opcode = canonical_opcode::RET_OP,
      },
    };

    detail::expect_compiled_code_block_matches(program.compiled_blocks[0], "main", false, detail::expected_machine_instructions_for_current_generator(expected_main));
    detail::expect_symbol_fixups_match(program.compiled_blocks[0].artifact, detail::expected_symbol_fixups_for_current_generator(expected_main));
  }

  TEST_F(vm_tests, ocmd_compiler_light_fuzzing) {
    detail::generator gen{};

    for (size_t iteration = 0; iteration < 128; ++iteration) {
      const detail::generated_program generated = detail::generate_simple_oasm_program(gen, iteration);
      SCOPED_TRACE(std::format("iteration={}\n{}", iteration, generated.source));

      diagnostic_engine diag;
      const auto program = detail::compile_source(generated.source, &diag);
      ASSERT_TRUE(program.valid);
      ASSERT_EQ(program.definitions.size(), generated.definitions.size());
      ASSERT_EQ(program.compiled_data_sections.size(), generated.data_blocks.size());
      ASSERT_EQ(program.compiled_blocks.size(), generated.code_blocks.size());

      for (size_t definition_index = 0; definition_index < generated.definitions.size(); ++definition_index) {
        detail::expect_definition_matches(
          program.definitions[definition_index], generated.definitions[definition_index].name, generated.definitions[definition_index].value);
      }
      for (size_t data_block_index = 0; data_block_index < generated.data_blocks.size(); ++data_block_index) {
        detail::expect_compiled_data_section_matches(
          program.compiled_data_sections[data_block_index],
          std::format("data_{}_{}", iteration, data_block_index),
          generated.data_blocks[data_block_index].objects);
      }
      for (size_t code_block_index = 0; code_block_index < generated.code_blocks.size(); ++code_block_index) {
        detail::expect_compiled_code_block_matches(
          program.compiled_blocks[code_block_index],
          generated.code_blocks[code_block_index].name,
          false,
          detail::expected_machine_instructions_for_current_generator(generated.code_blocks[code_block_index].instructions));
        detail::expect_symbol_fixups_match(
          program.compiled_blocks[code_block_index].artifact,
          detail::expected_symbol_fixups_for_current_generator(generated.code_blocks[code_block_index].instructions));
      }
    }
  }

  TEST_F(vm_tests, ocmd_compiler_rejects_ir_target_newer_than_generator) {
    const std::string source = R"(
    $main:
      ret
    end
    )";
    diagnostic_engine diag;
    const auto tokens = ocmd_lexer{ source }.tokenize(&diag);
    EXPECT_FALSE(tokens.empty()) << std::format("Tokenization failed for source:\n{}", source);

    const vm_version version{ 1, 0, 1 };
    const auto ir = oasm_parser{ version, tokens }.parse(&diag);
    ASSERT_TRUE(ir.valid);
    EXPECT_EQ(ir.code_blocks.size(), 1);
    EXPECT_EQ(ir.data_blocks.size(), 0);

    // version mismatch
    EXPECT_THROW(ocmd_compiler{ ir }.compile(make_scope<code_generator_000>(), &diag), ocmd_toolchain_error);
  }

}  // namespace other