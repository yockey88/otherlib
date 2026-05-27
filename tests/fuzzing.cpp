/**
 * \file tests/fuzzing.cpp
 **/
#include "fuzzing.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

namespace other {
  namespace detail {

    generator::generator() : gen(0x0A51F00Du),
                             data_block_count_dist(1, 3),
                             code_block_count_dist(1, 3),
                             object_count_dist(1, 3),
                             instruction_count_dist(1, 3),
                             data_kind_dist(0, 4),
                             instruction_kind_dist(0, 3),
                             register_dist(0, k_register_cases.size() - 1),
                             address_dist(0x0100, 0xFFFE),
                             integer_dist(1, 400),
                             should_define_dist(0, 1),
                             definition_dist(0, 3),
                             include_end_dist(0, 1),
                             blob_size_dist(1, 4),
                             byte_dist(0, 255),
                             label_dist() {}

    generated_program generate_simple_oasm_program(generator& gen, const size_t iteration) {
      constexpr size_t int_type_choice = 0;
      constexpr size_t string_type_choice = 1;
      constexpr size_t blob_type_choice = 2;
      constexpr size_t float_type_choice = 3;
      constexpr size_t address_type_choice = 4;

      generated_program program;
      std::vector<std::string> available_labels;

      const size_t data_block_count = gen.data_block_count_dist(gen.gen);
      for (size_t data_block_index = 0; data_block_index < data_block_count; ++data_block_index) {
        generated_data_block data_block_case;

        const bool should_gen_definitions = gen.should_define_dist(gen.gen) == 1;
        if (should_gen_definitions) {
          size_t definition_count = gen.definition_dist(gen.gen);
          for (size_t def_index = 0; def_index < definition_count; ++def_index) {
            const std::string def_name = "def_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(def_index);
            const std::string def_value = "value_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(def_index);
            program.definitions.push_back(generated_definition{
              .name = def_name,
              .value = def_value,
            });
            data_block_case.source += std::format("/{}:{}\n", def_name, def_value);
          }
        }

        data_block_case.source += std::format("#data_{}_{} {{\n", iteration, data_block_index);
        const size_t object_count = gen.object_count_dist(gen.gen);
        for (size_t object_index = 0; object_index < object_count; ++object_index) {
          const std::string object_name = "data_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(object_index);
          available_labels.push_back(object_name);

          switch (gen.data_kind_dist(gen.gen)) {
            case int_type_choice: {
              const int32_t integer_value = gen.integer_dist(gen.gen);
              data_block_case.source += "  ." + object_name + " : int32 = " + std::to_string(integer_value) + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = std::to_string(integer_value),
                .type = OCMD_DATA_TYPE_I32,
                .data = raw_bytes_from_value<int32_t>(static_cast<int32_t>(integer_value)),
              });
            } break;

            case string_type_choice: {
              const std::string string_value = "str_" + std::to_string(iteration) + "_" + std::to_string(data_block_index) + "_" + std::to_string(object_index);
              data_block_case.source += "  ." + object_name + " : string = \"" + string_value + "\"\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = string_value,
                .type = OCMD_DATA_TYPE_STRING,
                .data = raw_bytes_from_string(string_value),
              });
            } break;

            case blob_type_choice: {
              const size_t blob_size = gen.blob_size_dist(gen.gen);
              std::string blob_text;
              std::vector<uint8_t> blob_data;
              for (size_t byte_index = 0; byte_index < blob_size; ++byte_index) {
                const uint8_t byte = static_cast<uint8_t>(gen.byte_dist(gen.gen));
                if (!blob_text.empty()) {
                  blob_text += " ";
                }
                blob_text += hex_byte_string(byte);
                blob_data.push_back(byte);
              }

              data_block_case.source += "  ." + object_name + " : blob = " + blob_text + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = blob_text | std::views::split(' ') | std::views::join | std::ranges::to<std::string>(),
                .type = OCMD_DATA_TYPE_BLOB,
                .data = blob_data,
              });
            } break;

            case float_type_choice: {
              const float float_value = static_cast<float>(gen.integer_dist(gen.gen)) / 8.0f;
              std::string float_text = std::to_string(float_value);
              while (float_text.size() > 2 && float_text.back() == '0') {
                float_text.pop_back();
              }
              if (!float_text.empty() && float_text.back() == '.') {
                float_text.push_back('0');
              }

              data_block_case.source += "  ." + object_name + " : float = " + float_text + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = float_text,
                .type = OCMD_DATA_TYPE_F32,
                .data = raw_bytes_from_value<float>(std::stof(float_text)),
              });
            } break;

            case address_type_choice: {
              const uint16_t address_value = gen.address_dist(gen.gen);
              data_block_case.source += "  ." + object_name + " : address = 0x" + hex_word_string(address_value) + "\n";
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = "0x" + hex_word_string(address_value),
                .type = OCMD_DATA_TYPE_ADDRESS,
                .data = raw_bytes_from_value<uint16_t>(address_value),
              });
            } break;

            default: {
              throw std::logic_error("Invalid data kind choice");
            } break;
          }
        }

        data_block_case.source += "}\n";
        program.data_blocks.push_back(data_block_case);
      }

      const size_t code_block_count = gen.code_block_count_dist(gen.gen);

      for (size_t code_block_index = 0; code_block_index < code_block_count; ++code_block_index) {
        generated_code_block code_block_case;
        code_block_case.name = "code_" + std::to_string(iteration) + "_" + std::to_string(code_block_index);
        code_block_case.source += "$" + code_block_case.name + ":\n";

        const size_t instruction_count = gen.instruction_count_dist(gen.gen);
        for (size_t instruction_index = 0; instruction_index < instruction_count; ++instruction_index) {
          const auto& reg = get_register_case(gen.register_dist(gen.gen));
          switch (gen.instruction_kind_dist(gen.gen)) {
            case 0: {
              code_block_case.source += "  dump " + std::string{ reg.text } + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::DUMP_OP,
                .arguments = { make_register_argument(reg) },
              });
            } break;

            case 1: {
              const std::string& label = available_labels[gen.label_dist(gen.gen) % available_labels.size()];
              code_block_case.source += "  write " + std::string{ reg.text } + ", " + label + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::WRITE_OP,
                .arguments = { make_register_argument(reg), make_label_argument(label) },
              });
            } break;

            case 2: {
              const uint16_t address = gen.address_dist(gen.gen);
              code_block_case.source += "  set " + std::string{ reg.text } + ", 0x" + hex_word_string(address) + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_address_argument(address) },
              });
            } break;

            case 3: {
              const std::string& label = available_labels[gen.label_dist(gen.gen) % available_labels.size()];
              code_block_case.source += "  set " + std::string{ reg.text } + ", " + label + "\n";
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_label_argument(label) },
              });
            } break;

            default: {
              throw std::logic_error("Invalid instruction kind choice");
            } break;
          }
        }

        code_block_case.source += "  ret\n";
        code_block_case.instructions.push_back(expected_instruction{
          .expected_opcode = canonical_opcode::RET_OP,
        });

        if (gen.include_end_dist(gen.gen) == 1) {
          code_block_case.source += "end\n";
        }

        program.code_blocks.push_back(code_block_case);
      }

      const size_t max_sections = std::max(program.data_blocks.size(), program.code_blocks.size());
      for (size_t index = 0; index < max_sections; ++index) {
        if (index < program.data_blocks.size()) {
          program.source += program.data_blocks[index].source;
        }
        if (index < program.code_blocks.size()) {
          program.source += program.code_blocks[index].source;
        }
      }

      return program;
    }

    generated_program generate_simple_linkable_oasm_program(generator& gen, const size_t iteration) {
      constexpr size_t int_type_choice = 0;
      constexpr size_t string_type_choice = 1;
      constexpr size_t blob_type_choice = 2;
      constexpr size_t float_type_choice = 3;
      constexpr size_t address_type_choice = 4;

      constexpr size_t dump_instruction_choice = 0;
      constexpr size_t write_symbol_instruction_choice = 1;
      constexpr size_t set_address_instruction_choice = 2;
      constexpr size_t set_symbol_instruction_choice = 3;
      constexpr size_t call_symbol_instruction_choice = 4;

      generated_program program;
      std::vector<std::string> available_data_symbols;

      const size_t data_block_count = gen.data_block_count_dist(gen.gen);
      for (size_t data_block_index = 0; data_block_index < data_block_count; ++data_block_index) {
        generated_data_block data_block_case;
        const std::string section_name = std::format("data_{}_{}", iteration, data_block_index);

        const bool should_gen_definitions = gen.should_define_dist(gen.gen) == 1;
        if (should_gen_definitions) {
          const size_t definition_count = gen.definition_dist(gen.gen);
          for (size_t def_index = 0; def_index < definition_count; ++def_index) {
            const std::string def_name = std::format("def_{}_{}_{}", iteration, data_block_index, def_index);
            const std::string def_value = std::format("value_{}_{}_{}", iteration, data_block_index, def_index);
            program.definitions.push_back(generated_definition{
              .name = def_name,
              .value = def_value,
            });
            data_block_case.source += std::format("/{}:{}\n", def_name, def_value);
          }
        }

        data_block_case.source += std::format("#{} {{\n", section_name);
        const size_t object_count = gen.object_count_dist(gen.gen);
        for (size_t object_index = 0; object_index < object_count; ++object_index) {
          const std::string object_name = std::format("obj_{}_{}_{}", iteration, data_block_index, object_index);
          available_data_symbols.push_back(std::format("{}.{}", section_name, object_name));

          switch (gen.data_kind_dist(gen.gen)) {
            case int_type_choice: {
              const int32_t integer_value = gen.integer_dist(gen.gen);
              data_block_case.source += std::format("  .{} : int32 = {}\n", object_name, integer_value);
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = std::to_string(integer_value),
                .type = OCMD_DATA_TYPE_I32,
                .data = raw_bytes_from_value<int32_t>(integer_value),
              });
            } break;

            case string_type_choice: {
              const std::string string_value = std::format("str_{}_{}_{}", iteration, data_block_index, object_index);
              data_block_case.source += std::format("  .{} : string = \"{}\"\n", object_name, string_value);
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = string_value,
                .type = OCMD_DATA_TYPE_STRING,
                .data = raw_bytes_from_string(string_value),
              });
            } break;

            case blob_type_choice: {
              const size_t blob_size = gen.blob_size_dist(gen.gen);
              std::string blob_text;
              std::vector<uint8_t> blob_data;
              for (size_t byte_index = 0; byte_index < blob_size; ++byte_index) {
                const uint8_t byte = static_cast<uint8_t>(gen.byte_dist(gen.gen));
                if (!blob_text.empty()) {
                  blob_text += " ";
                }
                blob_text += hex_byte_string(byte);
                blob_data.push_back(byte);
              }

              data_block_case.source += std::format("  .{} : blob = {}\n", object_name, blob_text);
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = blob_text | std::views::split(' ') | std::views::join | std::ranges::to<std::string>(),
                .type = OCMD_DATA_TYPE_BLOB,
                .data = blob_data,
              });
            } break;

            case float_type_choice: {
              const float float_value = static_cast<float>(gen.integer_dist(gen.gen)) / 8.0f;
              std::string float_text = std::to_string(float_value);
              while (float_text.size() > 2 && float_text.back() == '0') {
                float_text.pop_back();
              }
              if (!float_text.empty() && float_text.back() == '.') {
                float_text.push_back('0');
              }

              data_block_case.source += std::format("  .{} : float = {}\n", object_name, float_text);
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = float_text,
                .type = OCMD_DATA_TYPE_F32,
                .data = raw_bytes_from_value<float>(std::stof(float_text)),
              });
            } break;

            case address_type_choice: {
              const uint16_t address_value = gen.address_dist(gen.gen);
              data_block_case.source += std::format("  .{} : address = 0x{}\n", object_name, hex_word_string(address_value));
              data_block_case.objects.push_back(expected_data_object{
                .name = object_name,
                .value_text = std::format("0x{}", hex_word_string(address_value)),
                .type = OCMD_DATA_TYPE_ADDRESS,
                .data = raw_bytes_from_value<uint16_t>(address_value),
              });
            } break;

            default: {
              throw std::logic_error("Invalid data kind choice");
            } break;
          }
        }

        data_block_case.source += "}\n";
        program.data_blocks.push_back(std::move(data_block_case));
      }

      const size_t code_block_count = gen.code_block_count_dist(gen.gen);
      std::vector<std::string> code_block_names;
      code_block_names.reserve(code_block_count);
      for (size_t code_block_index = 0; code_block_index < code_block_count; ++code_block_index) {
        code_block_names.push_back(std::format("code_{}_{}", iteration, code_block_index));
      }

      std::uniform_int_distribution<size_t> linker_instruction_kind_dist(0, call_symbol_instruction_choice);

      for (size_t code_block_index = 0; code_block_index < code_block_count; ++code_block_index) {
        generated_code_block code_block_case;
        code_block_case.name = code_block_names[code_block_index];
        code_block_case.source += std::format("${}:\n", code_block_case.name);

        const size_t instruction_count = gen.instruction_count_dist(gen.gen);
        for (size_t instruction_index = 0; instruction_index < instruction_count; ++instruction_index) {
          const auto& reg = get_register_case(gen.register_dist(gen.gen));
          switch (linker_instruction_kind_dist(gen.gen)) {
            case dump_instruction_choice: {
              code_block_case.source += std::format("  dump {}\n", reg.text);
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::DUMP_OP,
                .arguments = { make_register_argument(reg) },
              });
            } break;

            case write_symbol_instruction_choice: {
              const std::string& symbol = available_data_symbols[gen.label_dist(gen.gen) % available_data_symbols.size()];
              code_block_case.source += std::format("  write {}, {}\n", reg.text, symbol);
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::WRITE_OP,
                .arguments = { make_register_argument(reg), make_label_argument(symbol) },
              });
            } break;

            case set_address_instruction_choice: {
              const uint16_t address = gen.address_dist(gen.gen);
              code_block_case.source += std::format("  set {}, 0x{}\n", reg.text, hex_word_string(address));
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_address_argument(address) },
              });
            } break;

            case set_symbol_instruction_choice: {
              const std::string& symbol = available_data_symbols[gen.label_dist(gen.gen) % available_data_symbols.size()];
              code_block_case.source += std::format("  set {}, {}\n", reg.text, symbol);
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::SET_OP,
                .arguments = { make_register_argument(reg), make_label_argument(symbol) },
              });
            } break;

            case call_symbol_instruction_choice: {
              const std::string& symbol = code_block_names[gen.label_dist(gen.gen) % code_block_names.size()];
              code_block_case.source += std::format("  call {}\n", symbol);
              code_block_case.instructions.push_back(expected_instruction{
                .expected_opcode = canonical_opcode::CALL_OP,
                .arguments = { make_label_argument(symbol) },
              });
            } break;

            default: {
              throw std::logic_error("Invalid linker instruction kind choice");
            } break;
          }
        }

        code_block_case.source += "  ret\n";
        code_block_case.instructions.push_back(expected_instruction{
          .expected_opcode = canonical_opcode::RET_OP,
        });

        if (gen.include_end_dist(gen.gen) == 1) {
          code_block_case.source += "end\n";
        }

        program.code_blocks.push_back(std::move(code_block_case));
      }

      const size_t max_sections = std::max(program.data_blocks.size(), program.code_blocks.size());
      for (size_t index = 0; index < max_sections; ++index) {
        if (index < program.data_blocks.size()) {
          program.source += program.data_blocks[index].source;
        }
        if (index < program.code_blocks.size()) {
          program.source += program.code_blocks[index].source;
        }
      }

      return program;
    }

    generated_program generated_simple_bad_oasm_program(generator& gen, const size_t iteration) {
      constexpr size_t missing_data_close_before_code = 0;
      constexpr size_t missing_data_close_before_directive = 1;
      constexpr size_t wrong_load_arity = 2;
      constexpr size_t invalid_address_literal = 3;
      constexpr size_t duplicate_data_object_name = 4;

      generated_program program;

      const auto& reg_a = get_register_case(gen.register_dist(gen.gen));
      const auto& reg_b = get_register_case(gen.register_dist(gen.gen));
      const std::string code_name = std::format("bad_code_{}", iteration);
      const std::string object_name = std::format("bad_obj_{}", iteration);
      const std::string duplicate_name = std::format("dup_obj_{}", iteration);
      const uint16_t first_value = gen.integer_dist(gen.gen);
      const uint16_t second_value = gen.integer_dist(gen.gen);
      const uint16_t first_address = gen.address_dist(gen.gen);
      const uint16_t second_address = gen.address_dist(gen.gen);

      switch (iteration % 5) {
        case missing_data_close_before_code: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "${}:\n"
            "  dump {}\n"
            "  ret\n",
            object_name, first_value, code_name, reg_a.text
          );
          program.expected_error_substring = "Forgot to end data block, expected '}' before code block";
        } break;

        case missing_data_close_before_directive: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : string = \"broken_{}\"\n"
            "#data {{\n"
            "  .{}_next : int32 = {}\n"
            "}}\n"
            "${}:\n"
            "  ret\n",
            object_name, iteration, object_name, first_value, code_name
          );
          program.expected_error_substring = "Forgot to end data block, expected '}' before directive block";
        } break;

        case wrong_load_arity: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : address = 0x{}\n"
            "}}\n"
            "${}:\n"
            "  dump {}\n"
            "  load {}, 0x{}, {}\n"
            "  ret\n",
            object_name, hex_word_string(first_address), code_name, reg_a.text, reg_b.text, hex_word_string(second_address), reg_a.text
          );
          program.expected_error_substring = "Expected 2 parameters for instruction 'load', but found 3";
        } break;

        case invalid_address_literal: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : address = 0x{} 0x{}\n"
            "}}\n"
            "${}:\n"
            "  ret\n",
            object_name, hex_word_string(first_address), hex_word_string(second_address), code_name
          );
          program.expected_error_substring = "Expected a single hexadecimal literal for address type, but found 2 tokens";
        } break;

        case duplicate_data_object_name: {
          program.source = std::format(
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "}}\n"
            "${}:\n"
            "  dump {}\n"
            "  ret\n"
            "#data {{\n"
            "  .{} : int32 = {}\n"
            "}}\n",
            duplicate_name, first_value, code_name, reg_a.text, duplicate_name, second_value
          );
          program.expected_error_substring = std::format("Duplicate data object name '{}' in data block 'data'", duplicate_name);
        } break;

        default: {
          throw std::logic_error("Invalid negative test case choice");
        } break;
      }

      return program;
    }

  }  // namespace detail
}  // namespace other