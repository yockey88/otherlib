/**
 * \file vm/vm_tests.hpp
 **/
#ifndef OTHER_TESTS_VM_TESTS_HPP
#define OTHER_TESTS_VM_TESTS_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/command_files/data_block.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/ocmd_ir.hpp"
#include "vm/command_files/ocmd_program.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"

#include "other_test.hpp"

namespace other {
  namespace detail {

    struct expected_argument {
      token_type type = TOKEN_TYPE_INVALID;
      std::string raw_txt = "";
      bool indirect = false;
      std::optional<uint16_t> value = std::nullopt;
      std::vector<uint8_t> raw_data = {};
    };

    struct expected_instruction {
      canonical_opcode expected_opcode = canonical_opcode::INVALID_OP;
      std::vector<expected_argument> arguments = {};
    };

    struct expected_data_object {
      std::string name = "";
      std::string value_text = "";
      data_type type = OCMD_DATA_TYPE_INVALID;
      std::vector<uint8_t> data = {};
    };

    struct expected_definition {
      std::string name = "";
      std::string value = "";
    };

    struct register_case {
      std::string_view text;
      token_type type;
      uint16_t value;
    };

    struct expected_machine_instruction {
      uint32_t expected_opcode = 0;
    };

    constexpr std::array kRegisterKeywords = {
      register_case{ "r0", TOKEN_TYPE_KW_R0, vm_register_idx::VM_R0 },
      register_case{ "r1", TOKEN_TYPE_KW_R1, vm_register_idx::VM_R1 },
      register_case{ "r2", TOKEN_TYPE_KW_R2, vm_register_idx::VM_R2 },
      register_case{ "r3", TOKEN_TYPE_KW_R3, vm_register_idx::VM_R3 },
      register_case{ "r4", TOKEN_TYPE_KW_R4, vm_register_idx::VM_R4 },
      register_case{ "r5", TOKEN_TYPE_KW_R5, vm_register_idx::VM_R5 },
      register_case{ "r6", TOKEN_TYPE_KW_R6, vm_register_idx::VM_R6 },
      register_case{ "r7", TOKEN_TYPE_KW_R7, vm_register_idx::VM_R7 },
      register_case{ "r8", TOKEN_TYPE_KW_R8, vm_register_idx::VM_R8 },
      register_case{ "r9", TOKEN_TYPE_KW_R9, vm_register_idx::VM_R9 },
      register_case{ "ra", TOKEN_TYPE_KW_RA, vm_register_idx::VM_RA },
      register_case{ "rb", TOKEN_TYPE_KW_RB, vm_register_idx::VM_RB },
      register_case{ "rc", TOKEN_TYPE_KW_RC, vm_register_idx::VM_RC },
      register_case{ "rd", TOKEN_TYPE_KW_RD, vm_register_idx::VM_RD },
      register_case{ "re", TOKEN_TYPE_KW_RE, vm_register_idx::VM_RE },
      register_case{ "rf", TOKEN_TYPE_KW_RF, vm_register_idx::VM_RF },
      register_case{ "rflag", TOKEN_TYPE_KW_RFLAG, vm_register_idx::VM_RFLAG },
    };

    template <typename T>
    std::vector<uint8_t> raw_bytes_from_value(const T& value) {
      std::vector<uint8_t> data(sizeof(T));
      std::memcpy(data.data(), &value, sizeof(T));
      return data;
    }

    std::vector<uint8_t> raw_bytes_from_string(const std::string_view text);
    std::string hex_byte_string(const uint8_t value);
    std::string hex_word_string(const uint16_t value);
    ocmd_ir parse_source(const std::string_view source, diagnostic_engine* diag);
    ocmd_program compile_source(const std::string_view source, diagnostic_engine* diag);
    void expect_argument_matches(const raw_instruction::argument& actual, const expected_argument& expected);
    void expect_instruction_matches(const raw_instruction& actual, const expected_instruction& expected);
    void expect_code_block_matches(const code_block& actual, const std::string_view expected_name, const std::span<const expected_instruction> expected_instructions);
    void expect_data_block_matches(const data_block& actual, const std::string_view expected_name, const std::span<const expected_data_object> expected_objects);
    void expect_definition_matches(const compiler_definition& actual, const std::string_view expected_name, const std::string_view expected_value);
    void expect_machine_instruction_matches(const instruction& actual, const expected_machine_instruction& expected);
    void expect_compiled_code_block_matches(
      const compiled_code_block& actual, const std::string_view expected_name,
      const bool expected_is_entry_point, const std::span<const expected_machine_instruction> expected_instructions);
    const register_case& get_register_case(const size_t index);
    expected_argument make_register_argument(const register_case& reg);
    expected_argument make_integer_literal_argument(const int value);
    expected_argument make_address_argument(const uint16_t value, bool indirect = true);
    expected_argument make_label_argument(const std::string_view label, bool indirect = true);
    std::vector<expected_instruction> expected_foo_code_block();

    inline bool is_register_argument(const detail::expected_argument& argument) {
      return argument.type >= TOKEN_TYPE_KW_R0 && argument.type <= TOKEN_TYPE_KW_RFLAG && argument.value.has_value();
    }

    inline bool is_label_argument(const detail::expected_argument& argument) {
      return argument.type == TOKEN_TYPE_LABEL || argument.type == TOKEN_TYPE_IDENTIFIER;
    }

    inline uint8_t register_value(const detail::expected_argument& argument) {
      return static_cast<uint8_t>(argument.value.value());
    }

    inline uint16_t scalar_value(const detail::expected_argument& argument) {
      return argument.value.value();
    }

  }  // namespace detail

  class vm_tests : public other_test {
   public:
  };

}  // namespace other

#endif  // OTHER_TESTS_VM_TESTS_HPP