/**
 * \file vm/command_files/ocmd_linker.cpp
 **/
#include "vm/command_files/ocmd_linker.hpp"

#include <cstdint>
#include <ranges>

#include "core/logger.hpp"

#include "vm/command_files/compiler_error.hpp"
#include "vm/other_device.hpp"

namespace other {

  std::vector<uint8_t> ocmd_linker::link(scope<symbol_resolver> resolver) {
    OTHER_ASSERT(resolver != nullptr, "Symbol resolver scope cannot be null");

    /// resolve compiler gen symbol addresses by estimating start of section based on how man goto instructions we will have to add
    register_symbols(resolver);
    std::vector<uint8_t> compiler_generated_binary = create_compiler_generated_symbols(resolver);
    std::vector<uint8_t> linked_binary;

    rewrite_instructions(resolver);
    write_header(resolver, linked_binary);
    write_code(resolver, linked_binary);
    write_generated_code(resolver, linked_binary, compiler_generated_binary);
    write_data_sections(resolver, linked_binary);

    do_final_linking(resolver, linked_binary);

    return linked_binary;
  }

  void ocmd_linker::register_symbols(scope<symbol_resolver>& resolver) {
    for (const auto& data_section : code.compiled_data_sections) {
      resolver->register_symbol(data_section.name, {});
      for (const auto& field : data_section.fields) {
        resolver->register_symbol(std::format("{}.{}", data_section.name, field.name), {});
      }
    }

    for (const auto& code_block : code.compiled_blocks) {
      resolver->register_symbol(code_block.name, {});
    }
  }

  std::vector<uint8_t> ocmd_linker::create_compiler_generated_symbols(scope<symbol_resolver>& resolver) {
    std::vector<uint8_t> binary;
    for (const auto& code_block : code.compiled_blocks) {
      for (const auto& instr : code_block.artifact.unresolved_labels) {
        auto fixup = resolver->resolve_symbol(instr.symbol_name);
        if (!fixup.resolution_code.empty()) {
          /**
           * resolution works by replacing a resolved symbols fixup and instead of adding an address to the opcode,
           *  we insert a goto statement right before the opcode to the resolution code, we also insert a goto to go back to the right instruction
           *  after the resolution code. This allows us to support arbitrary resolution code without needing to worry about how much space it takes up or needing multiple passes to resolve everything
           *  the resolution code will be responsible for putting the resolved address in the right place in and will be compiled completely separately
           */
          const std::string gen_name = std::format("__compiler:{}", instr.symbol_name);
          resolver->register_symbol(gen_name, {});
          compiler_symbol_local_addresses.push_back({ gen_name, static_cast<uint16_t>(binary.size()) });

          // to the instruction address, the instruction will get moved one out
          instruction goto_instr = opcode_goto(instr.opcode_index * other_command_device::kOpCodeSize);

          binary.append_range(fixup.resolution_code);
          binary.append_range(opcode_to_bytes(goto_instr));
        }
      }
    }

    return binary;
  }

  void ocmd_linker::rewrite_instructions(scope<symbol_resolver>& resolver) {
    std::vector<uint8_t> unlinked_code_section;
    for (const auto& code_block : code.compiled_blocks) {
      auto bytes_view =
        code_block.artifact.machine_instructions |
        std::views::transform([&](const instruction& instr) { return opcode_to_bytes(instr); }) |
        std::views::join;
      unlinked_code_section.append_range(bytes_view);
    }

    uint16_t num_bytes_added_from_compiler_goto_gen = compiler_symbol_local_addresses.size() * sizeof(instruction);
    uint16_t real_compiler_gen_section_start = unlinked_code_section.size() + num_bytes_added_from_compiler_goto_gen;

    // go through and add a goto for each fixup that has resolution code
    for (auto& code_block : code.compiled_blocks) {
      for (const auto& instr : code_block.artifact.unresolved_labels) {
        const std::string sym_name = std::format("__compiler:{}", instr.symbol_name);
        auto gen_symbol = std::ranges::find(compiler_symbol_local_addresses, sym_name, &symbol_address::name);
        if (gen_symbol == compiler_symbol_local_addresses.end()) {
          continue;
        }

        resolver->attach_code_label(gen_symbol->name, real_compiler_gen_section_start + gen_symbol->local_address);
        auto fixup = resolver->resolve_symbol(gen_symbol->name);
        if (fixup.final_address == 0) {
          throw ocmd_linking_error(std::format("Failed to resolve compiler generated symbol '{}'", gen_symbol->name));
        }

        // add goto symbol to go the the resolution code
        instruction goto_instr = opcode_goto(fixup.final_address);

        // create new instruction list
        std::vector<instruction> artifact_copy = code_block.artifact.machine_instructions;
        auto before_gen_view = artifact_copy | std::views::take(instr.opcode_index);
        auto after_gen_view = artifact_copy | std::views::drop(instr.opcode_index);

        code_block.artifact.machine_instructions.clear();

        std::ranges::copy(before_gen_view, std::back_inserter(code_block.artifact.machine_instructions));
        code_block.artifact.machine_instructions.push_back(goto_instr);
        std::ranges::copy(after_gen_view, std::back_inserter(code_block.artifact.machine_instructions));
      }
    }
  }

  void ocmd_linker::write_header(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    std::vector<uint8_t> header = { 0x4F, 0x43, 0x4D, 0x44 };  // "OCMD" in ASCII
    binary.append_range(header);
  }

  void ocmd_linker::write_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (const auto& code_block : code.compiled_blocks) {
      auto bytes_view =
        code_block.artifact.machine_instructions |
        std::views::transform([&](const instruction& instr) { return opcode_to_bytes(instr); }) |
        std::views::join |
        std::ranges::to<std::vector>();
      resolver->attach_code_label(code_block.name, binary.size());
      binary.append_range(bytes_view);
    }
  }

  void ocmd_linker::write_generated_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary, std::span<const uint8_t> generated_code) {
    binary.append_range(generated_code);
  }

  void ocmd_linker::write_data_sections(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (const auto& data_section : code.compiled_data_sections) {
      resolver->attach_data_symbol(data_section.name, binary.size());
      for (const auto& field : data_section.fields) {
        const std::string full_field_name = std::format("{}.{}", data_section.name, field.name);
        resolver->attach_data_symbol(full_field_name, binary.size() + field.offset);
      }
      binary.append_range(data_section.data);
    }
  }

  void ocmd_linker::do_final_linking(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (auto& code_block : code.compiled_blocks) {
      // remove those we can, global linker later might remove more
      for (auto fixup_itr = code_block.artifact.unresolved_labels.begin(); fixup_itr != code_block.artifact.unresolved_labels.end();) {
        auto fixup = resolver->resolve_symbol(fixup_itr->symbol_name);
        if (fixup.final_address == 0) {
          ++fixup_itr;
        } else {
          // add one if goto was inserted
          const bool has_resolution_code = !fixup.resolution_code.empty();
          uint16_t binary_address = fixup_itr->opcode_index * other_command_device::kOpCodeSize + (has_resolution_code ? 1 : 0);

          uint8_t* instr_pointer = binary.data() + binary_address;
          instruction& instr = *reinterpret_cast<instruction*>(instr_pointer);
          instr.lower = fixup.final_address;
          fixup_itr = code_block.artifact.unresolved_labels.erase(fixup_itr);
        }
      }
    }
  }

}  // namespace other