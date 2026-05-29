/**
 * \file vm/ocmd_linker_tests.cpp
 **/
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "vm/command_files/ocmd_headers.hpp"
#include "vm/command_files/ocmd_linker.hpp"
#include "vm/default_symbol_resolver.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

#include "fuzzing.hpp"
#include "vm_tests.hpp"

namespace other {
  namespace detail {

    const ocmd_file_header& read_linked_header(const std::vector<uint8_t>& bytes);
    instruction read_instruction_at(const std::vector<uint8_t>& bytes, const uint16_t address);

    uint16_t linked_code_start();
    uint16_t linked_code_size(const ocmd_program& program, const std::vector<uint8_t>& bytes);
    uint16_t linked_data_start(const ocmd_program& program, const std::vector<uint8_t>& bytes);
    uint16_t code_block_address(const ocmd_program& program, const size_t block_index);

    std::unordered_map<std::string, uint16_t> collect_symbol_addresses(const ocmd_program& program, const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> collect_expected_data_bytes(const ocmd_program& program);

    void expect_linked_layout_matches_program(const ocmd_program& program, const std::vector<uint8_t>& bytes);
    void expect_linked_fixups_match_program(const ocmd_program& program, const std::vector<uint8_t>& bytes);

  }  // namespace detail

  TEST_F(vm_tests, ocmd_linker_correct_header_and_stopdev_guard) {
    const std::string_view source =
      R"(
    /entry: main
    #config {
      .threshold : int32 = 42
      .ratio : float = 1.5
    }
    $main:
      dump r1
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);

    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 1);
    ASSERT_EQ(program.compiled_data_sections.size(), 1);

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);

    detail::expect_linked_layout_matches_program(program, bytes);
  }

  TEST_F(vm_tests, ocmd_linker_multiple_code_blocks) {
    const std::string_view source =
      R"(
    $main:
      call helper
      ret
    end
    $helper:
      set r1, 7
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 2);
    ASSERT_TRUE(program.compiled_data_sections.empty());

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);

    detail::expect_linked_layout_matches_program(program, bytes);

    {
      ocmd_file_header header = detail::read_linked_header(bytes);
      EXPECT_EQ(header.file_version_major, OCMD_FILE_FORMAT_VERSION_MAJOR);
      EXPECT_EQ(header.file_version_minor, OCMD_FILE_FORMAT_VERSION_MINOR);
      EXPECT_EQ(header.file_version_patch, OCMD_FILE_FORMAT_VERSION_PATCH);
      EXPECT_EQ(header.prog_header.code_section_offset, detail::linked_code_start());
      EXPECT_EQ(header.prog_header.data_section_offset, detail::linked_data_start(program, bytes));
    }

    const uint16_t main_address = detail::code_block_address(program, 0);
    const uint16_t helper_address = detail::code_block_address(program, 1);

    instruction main_first_instr = detail::read_instruction_at(bytes, main_address);
    instruction expected_first = opcode_call_at(helper_address);

    instruction main_second_instr = detail::read_instruction_at(bytes, main_address + sizeof(instruction));
    instruction expected_second = opcode_return();

    instruction helper_first_instr = detail::read_instruction_at(bytes, helper_address);
    instruction expected_helper_first = opcode_load_x_direct(vm_register_idx::VM_R1, 7);

    instruction helper_second_instr = detail::read_instruction_at(bytes, helper_address + sizeof(instruction));
    instruction expected_helper_second = opcode_return();

    std::stringstream ss;
    for (size_t i = 0; i < bytes.size(); i += sizeof(instruction)) {
      ss << std::format("{:#08x}: {:#010x}", i, detail::read_instruction_at(bytes, i).opcode);
      ss << "\n";
    }

    {
      SCOPED_TRACE(std::format("main1 set:\n{}", ss.str()));
      EXPECT_EQ(main_first_instr.opcode, expected_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_first.opcode, main_address, main_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_first_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("main2 ret:\n{}", ss.str()));
      EXPECT_EQ(main_second_instr.opcode, expected_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_second.opcode, main_address + sizeof(instruction), main_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_second_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("helper1 load:\n{}", ss.str()));
      EXPECT_EQ(helper_first_instr.opcode, expected_helper_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_first.opcode, helper_address, helper_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_first_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("helper2 ret:\n{}", ss.str()));
      EXPECT_EQ(helper_second_instr.opcode, expected_helper_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_second.opcode, helper_address + sizeof(instruction), helper_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_second_instr.opcode);
    }
  }

  TEST_F(vm_tests, ocmd_linker_multiple_code_blocks_w_data) {
    const std::string_view source =
      R"(
    #data {
      .number : int32 = 7
    }
    $main:
      call helper
      ret
    end
    $helper:
      set r1, data.number
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 2);
    ASSERT_EQ(program.compiled_data_sections.size(), 1);

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);

    detail::expect_linked_layout_matches_program(program, bytes);

    {
      ocmd_file_header header = detail::read_linked_header(bytes);
      EXPECT_EQ(header.file_version_major, OCMD_FILE_FORMAT_VERSION_MAJOR);
      EXPECT_EQ(header.file_version_minor, OCMD_FILE_FORMAT_VERSION_MINOR);
      EXPECT_EQ(header.file_version_patch, OCMD_FILE_FORMAT_VERSION_PATCH);
      EXPECT_EQ(header.prog_header.code_section_offset, detail::linked_code_start());
      EXPECT_EQ(header.prog_header.data_section_offset, detail::linked_data_start(program, bytes));
    }

    const uint16_t main_address = detail::code_block_address(program, 0);
    const uint16_t helper_address = detail::code_block_address(program, 1);

    instruction main_first_instr = detail::read_instruction_at(bytes, main_address);
    instruction expected_first = opcode_call_at(helper_address);

    instruction main_second_instr = detail::read_instruction_at(bytes, main_address + sizeof(instruction));
    instruction expected_second = opcode_return();

    instruction helper_first_instr = detail::read_instruction_at(bytes, helper_address);
    instruction expected_helper_first = opcode_load_x_from(vm_register_idx::VM_R1, 0x0034);

    instruction helper_second_instr = detail::read_instruction_at(bytes, helper_address + sizeof(instruction));
    instruction expected_helper_second = opcode_return();
    {
      std::stringstream ss;
      {
        ss << "Linked binary hexdump:\n";
        for (size_t i = 0; i < bytes.size(); i += sizeof(instruction)) {
          ss << std::format("{:#08x}: {:#010x}", i, detail::read_instruction_at(bytes, i).opcode);
          ss << "\n";
        }
      }
      SCOPED_TRACE(ss.str());
      EXPECT_EQ(main_first_instr.opcode, expected_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_first.opcode, main_address, main_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_first_instr.opcode);
      EXPECT_EQ(main_second_instr.opcode, expected_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_second.opcode, main_address + sizeof(instruction), main_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_second_instr.opcode);
      EXPECT_EQ(helper_first_instr.opcode, expected_helper_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_first.opcode, helper_address, helper_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_first_instr.opcode);
      EXPECT_EQ(helper_second_instr.opcode, expected_helper_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_second.opcode, helper_address + sizeof(instruction), helper_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_second_instr.opcode);
    }

    // check data section
    const uint16_t data_start = detail::linked_data_start(program, bytes);

    int32_t linked_number = *reinterpret_cast<const int32_t*>(bytes.data() + data_start);
    EXPECT_EQ(linked_number, 7) << std::format("Expected linked data number to be 7, but found {}", linked_number);
  }

  TEST_F(vm_tests, ocmd_linker_linking_multiple_sections) {
    const std::string_view source =
      R"(
    #alpha {
      .flag : address = 0x1234
    }
    #beta {
      .payload : blob = DE AD BE EF
    }
    $main:
      set r1, beta.payload
      ret
    end
    $helper:
      set r3, alpha.flag
      ret
    end
    )";

    CORE_LOG_DEBUG("SOURCE:{}", source);
    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);

    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 2);
    ASSERT_EQ(program.compiled_data_sections.size(), 2);

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);

    constexpr size_t expected_code_size = 4 * sizeof(instruction) + sizeof(instruction);  // 4 instructions in total across both code blocks, then stopdev
    constexpr size_t expected_data_size = 2 * sizeof(natural_t);                          // 2 64 bit data objects (address and blob)
    ocmd_file_header expected_header{
      .file_signature = { 'O', 'C', 'M', 'D' },
      .file_version_major = OCMD_FILE_FORMAT_VERSION_MAJOR,
      .file_version_minor = OCMD_FILE_FORMAT_VERSION_MINOR,
      .file_version_patch = OCMD_FILE_FORMAT_VERSION_PATCH,
      .prog_header = {
        .has_code_flag = 1,
        .code_section_offset = sizeof(ocmd_file_header),
        .data_section_offset = sizeof(ocmd_file_header) + expected_code_size,                     // 4 instructions in code section
        .data_table_offset = sizeof(ocmd_file_header) + expected_code_size + expected_data_size,  // data section has 8 bytes (4 for address, 4 for blob, 1 for stopdev)
        .entry_point_address = sizeof(ocmd_file_header),
        .num_instructions = 4,
      },
    };

    {
      const auto& header = detail::read_linked_header(bytes);
      ASSERT_EQ(std::string_view(header.file_signature, sizeof(header.file_signature)), "OCMD");
      ASSERT_EQ(header.file_version_major, OCMD_FILE_FORMAT_VERSION_MAJOR);
      ASSERT_EQ(header.file_version_minor, OCMD_FILE_FORMAT_VERSION_MINOR);
      ASSERT_EQ(header.file_version_patch, OCMD_FILE_FORMAT_VERSION_PATCH);
      ASSERT_EQ(header.prog_header.code_section_offset, expected_header.prog_header.code_section_offset);
      ASSERT_EQ(header.prog_header.data_section_offset, expected_header.prog_header.data_section_offset);
    }

    detail::expect_linked_layout_matches_program(program, bytes);

    const auto symbol_addresses = detail::collect_symbol_addresses(program, bytes);
    const uint16_t main_address = detail::code_block_address(program, 0);
    const uint16_t main_ret_address = main_address + sizeof(instruction);

    const uint16_t helper_address = detail::code_block_address(program, 1);
    const uint16_t helper_ret_address = helper_address + sizeof(instruction);

    instruction main_first_instr = detail::read_instruction_at(bytes, main_address);
    instruction expected_first = opcode_load_x_from(vm_register_idx::VM_R1, symbol_addresses.at("beta.payload"));

    instruction main_second_instr = detail::read_instruction_at(bytes, main_ret_address);
    instruction expected_second = opcode_return();

    instruction helper_first_instr = detail::read_instruction_at(bytes, helper_address);
    instruction expected_helper_first = opcode_load_x_from(vm_register_idx::VM_R3, symbol_addresses.at("alpha.flag"));

    instruction helper_second_instr = detail::read_instruction_at(bytes, helper_ret_address);
    instruction expected_helper_second = opcode_return();

    std::stringstream ss;
    for (size_t i = 0; i < bytes.size(); i += sizeof(instruction)) {
      uint32_t val = *reinterpret_cast<const uint32_t*>(bytes.data() + i);
      ss << std::format("{:#08x}: {:#010x}", i, val);
      ss << "\n";
    }

    {
      SCOPED_TRACE(std::format("main set:\n{}", ss.str()));
      EXPECT_EQ(main_first_instr.opcode, expected_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_first.opcode, main_address, main_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_first_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("main ret:\n{}", ss.str()));
      EXPECT_EQ(main_second_instr.opcode, expected_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_second.opcode, main_ret_address, main_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_second_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("helper set:\n{}", ss.str()));
      EXPECT_EQ(helper_first_instr.opcode, expected_helper_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_first.opcode, helper_address, helper_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_first_instr.opcode);
    }
    {
      SCOPED_TRACE(std::format("helper ret:\n{}", ss.str()));
      EXPECT_EQ(helper_second_instr.opcode, expected_helper_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_helper_second.opcode, helper_ret_address, helper_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", helper_second_instr.opcode);
    }

    // check data sections
    const uint16_t alpha_flag_address = symbol_addresses.at("alpha.flag");
    const uint16_t beta_payload_address = symbol_addresses.at("beta.payload");

    // address type
    uint16_t linked_alpha_flag = *reinterpret_cast<const uint16_t*>(bytes.data() + alpha_flag_address);
    EXPECT_EQ(linked_alpha_flag, 0x1234)
      << std::format("Expected linked alpha.flag to be 0x1234, but found {:#06x}", linked_alpha_flag);

    uint8_t linked_beta_payload[4];
    std::memcpy(linked_beta_payload, bytes.data() + beta_payload_address, 4);
    EXPECT_EQ(std::memcmp(linked_beta_payload, "\xDE\xAD\xBE\xEF", 4), 0)
      << "Expected linked beta.payload to be DE AD BE EF, but found something else.";
  }

  TEST_F(vm_tests, ocmd_linker_symbol_resolution) {
    const std::string_view source =
      R"(
    $main:
      call host.api
      set r1, host.api
      write r2, host.api
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 1);

    auto resolver = make_scope<default_symbol_resolver>();
    resolver->attach_code_label("host.api", 0x4321);

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(std::move(resolver), &diag);

    detail::expect_linked_layout_matches_program(program, bytes);

    const uint16_t main_address = detail::code_block_address(program, 0);
    instruction main_first_instr = detail::read_instruction_at(bytes, main_address);
    instruction main_second_instr = detail::read_instruction_at(bytes, main_address + sizeof(instruction));
    instruction main_third_instr = detail::read_instruction_at(bytes, main_address + 2 * sizeof(instruction));

    instruction expected_first = opcode_call_at(0x4321);
    instruction expected_second = opcode_load_x_from(vm_register_idx::VM_R1, 0x4321);
    instruction expected_third = opcode_write_x_to_memory(vm_register_idx::VM_R2, 0x4321);

    {
      std::stringstream ss;
      ss << "Linked binary hexdump:\n";
      for (size_t i = 0; i < bytes.size(); i += sizeof(instruction)) {
        ss << std::format("{:#08x}: {:#010x}", i, detail::read_instruction_at(bytes, i).opcode);
        ss << "\n";
      }
      SCOPED_TRACE(ss.str());
      EXPECT_EQ(main_first_instr.opcode, expected_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_first.opcode, main_address, main_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_first_instr.opcode);
      EXPECT_EQ(main_second_instr.opcode, expected_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_second.opcode, main_address + sizeof(instruction), main_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_second_instr.opcode);
      EXPECT_EQ(main_third_instr.opcode, expected_third.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_third.opcode, main_address + 2 * sizeof(instruction), main_third_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_third_instr.opcode);
    }
  }

  TEST_F(vm_tests, ocmd_linker_leaves_unresolved_placeholders) {
    const std::string_view source =
      R"(
    $main:
      call missing.api
      set r1, missing.api
      write r2, missing.api
      ret
    end
    )";

    diagnostic_engine diag;
    const auto program = detail::compile_source(source, &diag);
    ASSERT_TRUE(program.valid);
    ASSERT_EQ(program.compiled_blocks.size(), 1);

    const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(make_scope<default_symbol_resolver>(), &diag);

    detail::expect_linked_layout_matches_program(program, bytes);

    const uint16_t main_address = detail::code_block_address(program, 0);
    instruction main_first_instr = detail::read_instruction_at(bytes, main_address);
    instruction main_second_instr = detail::read_instruction_at(bytes, main_address + sizeof(instruction));
    instruction main_third_instr = detail::read_instruction_at(bytes, main_address + 2 * sizeof(instruction));

    instruction expected_first = opcode_call_at(0xFFFF);
    instruction expected_second = opcode_load_x_from(vm_register_idx::VM_R1, 0xFFFF);
    instruction expected_third = opcode_write_x_to_memory(vm_register_idx::VM_R2, 0xFFFF);

    {
      std::stringstream ss;
      ss << "Linked binary hexdump:\n";
      for (size_t i = 0; i < bytes.size(); i += sizeof(instruction)) {
        ss << std::format("{:#08x}: {:#010x}", i, detail::read_instruction_at(bytes, i).opcode);
        ss << "\n";
      }
      SCOPED_TRACE(ss.str());
      EXPECT_EQ(main_first_instr.opcode, expected_first.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_first.opcode, main_address, main_first_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_first_instr.opcode);
      EXPECT_EQ(main_second_instr.opcode, expected_second.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_second.opcode, main_address + sizeof(instruction), main_second_instr.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_second_instr.opcode);
      EXPECT_EQ(main_third_instr.opcode, expected_third.opcode)
        << std::format("Expected instruction {:#010x} at address 0x{:04X},", expected_third.opcode, main_address + 2 * sizeof(instruction), expected_third.opcode)
        << std::format(" Found instruction {:#010x} instead.", main_third_instr.opcode);
    }
  }

  TEST_F(vm_tests, ocmd_linker_light_fuzzing) {
    detail::generator gen{};

    for (size_t iteration = 0; iteration < 128; ++iteration) {
      const detail::generated_program generated = detail::generate_simple_linkable_oasm_program(gen, iteration);
      SCOPED_TRACE(std::format("iteration={}\n{}", iteration, generated.source));

      diagnostic_engine diag;
      const auto program = detail::compile_source(generated.source, &diag);
      ASSERT_TRUE(program.valid);
      ASSERT_EQ(program.definitions.size(), generated.definitions.size());
      ASSERT_EQ(program.compiled_data_sections.size(), generated.data_blocks.size());
      ASSERT_EQ(program.compiled_blocks.size(), generated.code_blocks.size());

      auto resolver = make_scope<default_symbol_resolver>();
      const std::vector<uint8_t> bytes = ocmd_linker{ program }.link(std::move(resolver), &diag);

      detail::expect_linked_layout_matches_program(program, bytes);
      detail::expect_linked_fixups_match_program(program, bytes);
    }
  }

  namespace detail {

    const ocmd_file_header& read_linked_header(const std::vector<uint8_t>& bytes) {
      EXPECT_GE(bytes.size(), sizeof(ocmd_file_header));
      return *reinterpret_cast<const ocmd_file_header*>(bytes.data());
    }

    instruction read_instruction_at(const std::vector<uint8_t>& bytes, const uint16_t address) {
      uint32_t opcode = 0;
      if (bytes.size() < address + sizeof(instruction)) {
        ADD_FAILURE() << std::format("Attempted to read instruction at address {:#04x}, but linked binary only has {} bytes", address, bytes.size());
        return instruction{ 0 };
      }
      std::memcpy(&opcode, bytes.data() + address, sizeof(opcode));
      return instruction{ opcode };
    }

    uint16_t linked_code_start() {
      return static_cast<uint16_t>(sizeof(ocmd_file_header));
    }

    uint16_t linked_code_size(const ocmd_program& program, const std::vector<uint8_t>& bytes) {
      size_t total_size = bytes.size();
      size_t sections_only = total_size - sizeof(ocmd_file_header);
      size_t data_size = collect_expected_data_bytes(program).size();
      return static_cast<uint16_t>(sections_only - data_size);
    }

    uint16_t linked_data_start(const ocmd_program& program, const std::vector<uint8_t>& bytes) {
      size_t data_size = collect_expected_data_bytes(program).size();
      return static_cast<uint16_t>(bytes.size() - data_size);
    }

    uint16_t code_block_address(const ocmd_program& program, const size_t block_index) {
      uint16_t offset = linked_code_start();
      for (size_t i = 0; i < block_index; ++i) {
        offset += program.compiled_blocks[i].artifact.machine_instructions.size() * sizeof(instruction);
      }
      return offset;
    }

    std::unordered_map<std::string, uint16_t> collect_symbol_addresses(const ocmd_program& program, const std::vector<uint8_t>& bytes) {
      std::unordered_map<std::string, uint16_t> addresses;

      for (size_t block_index = 0; block_index < program.compiled_blocks.size(); ++block_index) {
        addresses.emplace(program.compiled_blocks[block_index].name, code_block_address(program, block_index));
      }

      uint16_t section_address = linked_data_start(program, bytes);
      for (const auto& data_section : program.compiled_data_sections) {
        addresses.emplace(data_section.name, section_address);
        for (const auto& field : data_section.fields) {
          addresses.emplace(std::format("{}.{}", data_section.name, field.name), static_cast<uint16_t>(section_address + field.offset));
        }
        section_address += static_cast<uint16_t>(data_section.data.size());
      }

      return addresses;
    }

    std::vector<uint8_t> collect_expected_data_bytes(const ocmd_program& program) {
      std::vector<uint8_t> data_bytes;
      for (const auto& data_section : program.compiled_data_sections) {
        data_bytes.append_range(data_section.data);
      }
      return data_bytes;
    }

    void expect_linked_layout_matches_program(const ocmd_program& program, const std::vector<uint8_t>& bytes) {
      const auto& header = read_linked_header(bytes);
      {
        uint16_t address = linked_code_start() + (header.prog_header.num_instructions * sizeof(instruction)) - sizeof(instruction);
        const instruction stopdev = read_instruction_at(bytes, address);
        EXPECT_EQ(stopdev.opcode, opcode_stop_device());
      }

      auto expected_data = collect_expected_data_bytes(program);
      std::vector<uint8_t> actual_data(bytes.begin() + header.prog_header.data_section_offset, bytes.end());
      ASSERT_EQ(actual_data.size(), expected_data.size()) << std::format("Expected data section size {} bytes, but found {} bytes", expected_data.size(), actual_data.size());

      std::stringstream ss1;
      std::stringstream ss2;

      {
        {
          ss1 << "Actual data section:\n";
          for (size_t i = 0; i < actual_data.size(); i += sizeof(instruction)) {
            uint32_t val = *reinterpret_cast<const uint32_t*>(actual_data.data() + i);
            ss1 << std::format("{:#08x}: {:#010x}", i, val);
            ss1 << "\n";
          }
        }
        {
          ss2 << "Expected data bytes:\n";
          for (size_t i = 0; i < expected_data.size(); i += sizeof(instruction)) {
            uint32_t val = *reinterpret_cast<const uint32_t*>(expected_data.data() + i);
            ss2 << std::format("{:#08x}: {:#010x}", i, val);
            ss2 << "\n";
          }
        }

        SCOPED_TRACE(ss1.str());
        SCOPED_TRACE(ss2.str());
        EXPECT_TRUE(std::ranges::equal(actual_data, std::span(expected_data)));
      }
    }

    void expect_linked_fixups_match_program(const ocmd_program& program, const std::vector<uint8_t>& bytes) {
      const auto symbol_addresses = collect_symbol_addresses(program, bytes);

      for (size_t block_index = 0; block_index < program.compiled_blocks.size(); ++block_index) {
        const auto& block = program.compiled_blocks[block_index];
        std::unordered_map<size_t, std::string> fixups_by_instruction;
        for (const auto& fixup : block.artifact.unresolved_labels) {
          auto [_, inserted] = fixups_by_instruction.emplace(fixup.opcode_index, fixup.symbol_name);
          EXPECT_TRUE(inserted) << std::format("Duplicate fixup for block '{}' instruction {}", block.name, fixup.opcode_index);
        }

        const uint16_t block_address = code_block_address(program, block_index);
        for (size_t instruction_index = 0; instruction_index < block.artifact.machine_instructions.size(); ++instruction_index) {
          instruction expected = block.artifact.machine_instructions[instruction_index];
          if (const auto fixup_itr = fixups_by_instruction.find(instruction_index); fixup_itr != fixups_by_instruction.end()) {
            const auto symbol_itr = symbol_addresses.find(fixup_itr->second);
            ASSERT_NE(symbol_itr, symbol_addresses.end()) << std::format("Missing resolved symbol '{}'", fixup_itr->second);
            expected.lower = symbol_itr->second;
          }

          const instruction actual = read_instruction_at(bytes, static_cast<uint16_t>(block_address + instruction_index * sizeof(instruction)));
          EXPECT_EQ(actual.opcode, expected.opcode)
            << std::format("block='{}' instruction_index={} linked_address=0x{:04X}", block.name, instruction_index, block_address + instruction_index * sizeof(instruction));
        }
      }
    }

  }  // namespace detail
}  // namespace other