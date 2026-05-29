/**
 * \file vm/vm_tests.cpp
 **/
#include "vm/vm_tests.hpp"

#include <gtest/gtest.h>

#include "vm/code_generator_000.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/command_files/ocmd_linker.hpp"
#include "vm/control_table.hpp"
#include "vm/default_symbol_resolver.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/ocmd_trace_sink.hpp"
#include "vm/vm.hpp"

namespace other {
  namespace detail {

    std::string get_test_program1_source();

  }  // namespace detail

  TEST_F(vm_tests, basic_initialization_and_shutdown) {
    // GTEST_SKIP() << "Not Working";
    other_command_device device;

    ASSERT_NO_FATAL_FAILURE(vm::initialize_device(&device));
    ASSERT_NE(device.memory, nullptr);
    ASSERT_NE(device.control_table, nullptr);

    ocmd_file_header header = {
      .file_signature = { 'O', 'C', 'M', 'D' },
      .file_version_major = 1,
      .file_version_minor = 0,
      .file_version_patch = 0,
      .prog_header = {
        .has_code_flag = 1,
        .code_section_offset = sizeof(ocmd_file_header),
        .code_size = 16,
        .data_section_offset = 0,
        .data_size = 0,
        .data_table_offset = 0,
        .entry_point_address = sizeof(ocmd_file_header),
        .num_instructions = 4,
      },
      .reserved = { 0 },
    };
    const std::vector<uint8_t> bytes = {
      0x00, 0x01, 0x02, 0x03,
      0x10, 0x11, 0x12, 0x13,
      0x20, 0x21, 0x22, 0x23,
      0x30, 0x31, 0x32, 0x33
    };

    std::stringstream ss;
    ss << "Program Bytes:\n";
    for (size_t i = 0; i < bytes.size(); i += 4) {
      uint32_t opcode = *reinterpret_cast<const uint32_t*>(bytes.data() + i);
      ss << std::format("{:#04x} ", opcode);
    }

    std::vector<uint8_t> linked_binary;
    linked_binary.reserve(sizeof(ocmd_file_header) + bytes.size());

    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    linked_binary.append_range(std::span(header_bytes, sizeof(ocmd_file_header)));
    linked_binary.append_range(bytes);
    ASSERT_NO_FATAL_FAILURE(vm::load_program_from_bytes(&device, linked_binary));

    const auto& meta = device.current_program_metadata;

    uint32_t idx = 0;
    for (const auto b : std::span(device.memory->data + meta.load_address, bytes.size())) {
      EXPECT_EQ(b, bytes[idx]) << std::format("Bytes at {} are not equal: {} != {}", idx, b, bytes[idx]);
      idx++;
    }

    ASSERT_NO_FATAL_FAILURE(vm::shutdown_device(&device));
  }

  TEST_F(vm_tests, vm_program1) {
    std::string program1_src = detail::get_test_program1_source();
    CORE_LOG_DEBUG("Program Source:{}", program1_src);
    std::vector<uint8_t> bytes = {};

    diagnostic_engine diag;
    {
      ocmd_trace_sink ts;
      natural_t ts_id = diag.register_sink("tracer", &ts);

      auto tokens = ocmd_lexer{ program1_src }.tokenize(&diag);
      ASSERT_FALSE(tokens.empty())
        << std::format("Tokenization failed for source:\n{}", program1_src);

      auto ir = oasm_parser{ vm_version{}, tokens }.parse(&diag);
      ASSERT_TRUE(ir.valid)
        << std::format("Parsing failed for source:\n{}", program1_src);
      auto program = ocmd_compiler{ ir }.compile(make_scope<code_generator_000>(), &diag);
      ASSERT_TRUE(program.valid)
        << std::format("Compilation failed for source:\n{}", program1_src);

      bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);
      diag.remove_sink(ts_id);
    }

    ASSERT_FALSE(bytes.empty());

    other_command_device device;
    ASSERT_NO_FATAL_FAILURE(vm::initialize_device(&device));
    ASSERT_NE(device.memory, nullptr);
    ASSERT_NE(device.control_table, nullptr);

    ASSERT_NO_FATAL_FAILURE(vm::load_program_from_bytes(&device, bytes));

    {
      std::stringstream ss;
      ss << "Running program...\n";
      for (uint16_t addr = sizeof(ocmd_file_header); addr < bytes.size(); addr += other_command_device::kOpCodeSize) {
        const instruction instr = *reinterpret_cast<const instruction*>(bytes.data() + addr);
        ss << std::format("{:#04x} : {:#010x}\n", addr, instr.opcode);
      }
      CORE_LOG_DEBUG("{}", ss.str());
      do {
        vm::step(&device);
      } while (!device.stopped);
    }

    natural_t r1_value = device.registers[vm_register_idx::VM_R1].memory.to_u64();
    EXPECT_EQ(r1_value, 42) << std::format("Expected R1 to be 42, but got {}", r1_value);

    ASSERT_NO_FATAL_FAILURE(vm::shutdown_device(&device));
  }

  namespace detail {

    std::string get_test_program1_source() {
      return R"(
      #data {
        .number : int32 = 42
      }
      $main:
        set r1, data.number
        dump r1
      end
      )";
    }

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

    ocmd_ir parse_source(const std::string_view source, diagnostic_engine* diag) {
      const auto tokens = ocmd_lexer{ source }.tokenize(diag);
      EXPECT_FALSE(tokens.empty())
        << std::format("Tokenization failed for source:\n{}", source);
      return oasm_parser{ vm_version{}, tokens }.parse(diag);
    }

    ocmd_program compile_source(const std::string_view source, diagnostic_engine* diag) {
      auto ir = parse_source(source, diag);
      if (!ir.valid) {
        return {};
      }

      return ocmd_compiler{ ir }.compile(make_scope<code_generator_000>(), diag);
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
      const bool expected_is_entry_point, const std::span<const expected_machine_instruction> expected_instructions) {
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