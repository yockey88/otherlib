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

    /**
     * Layout:
     * | Header | Compiler Generated Code | Code | Data Sections |
     **/
    std::vector<uint8_t> linked_binary;
    // Header
    write_header(resolver, linked_binary);
    // Compiler Generated Code
    // Code
    {
      /// \todo go through and see if any symbols have invocation thunks, and if so, generate the code and
      ///        add gotos like so:
      /**
       * $symbol:
       *   call my_symbol_w_thunk
       *   ....
       *
       * will expand to:
       * $symbol:
       *   goto __compiler:my_symbol_w_thunk
       *   @__compiler:my_symbol_w_thunk_invocation_ready
       *   call my_symbol_w_thunk
       *
       *
       * __compiler:my_symbol_w_thunk:
       *   <invocation thunk code>
       *   goto __compiler:my_symbol_w_thunk_invocation_ready
       **/
      // std::vector<uint8_t> compiler_generated_binary = create_compiler_generated_symbols(resolver);
      // rewrite_instructions(resolver);
      // write_generated_code(resolver, linked_binary, compiler_generated_binary);
      write_code(resolver, linked_binary);
      // add a stopdev at the end of code section to prevent accidental execution of data if entry point is not set correctly
      linked_binary.append_range(opcode_to_bytes(opcode_stop_device()));

      // modify header
      ocmd_file_header& header = *reinterpret_cast<ocmd_file_header*>(linked_binary.data());
      header.prog_header.code_size = static_cast<uint16_t>(linked_binary.size() - sizeof(ocmd_file_header));
      header.prog_header.data_section_offset = linked_binary.size();
      CORE_LOG_DEBUG("[SIZE]: {} bytes", linked_binary.size());
    }

    // Data Section
    write_data_sections(resolver, linked_binary);

    // modify header
    {
      ocmd_file_header& header = *reinterpret_cast<ocmd_file_header*>(linked_binary.data());
      header.prog_header.data_size = static_cast<uint16_t>(linked_binary.size() - header.prog_header.data_section_offset);

      /// \todo: header.prog_header.entry_point_address = resolver->resolve_symbol("main").final_address;
      header.prog_header.entry_point_address = header.prog_header.code_section_offset;

      std::span code_view{ linked_binary.data() + sizeof(ocmd_file_header), header.prog_header.code_size };
      uint16_t instruction_count = static_cast<uint16_t>(code_view.size() / sizeof(instruction));
      header.prog_header.num_instructions = instruction_count;
    }

    CORE_LOG_DEBUG("[SIZE]: {} bytes", linked_binary.size());
    do_final_linking(resolver, linked_binary);
    return linked_binary;
  }

  uint16_t ocmd_linker::calculate_code_section_offset(size_t index) const {
    size_t offset = sizeof(ocmd_file_header);
    for (size_t i = 0; i < index; ++i) {
      offset += code.compiled_blocks[i].artifact.machine_instructions.size() * sizeof(instruction);
    }
    return static_cast<uint16_t>(offset);
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

  void ocmd_linker::write_header(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    ocmd_file_header header{
      // .file_signature = { 'O', 'C', 'M', 'D' },
      // .file_version_major = OCMD_FILE_FORMAT_VERSION_MAJOR,
      // .file_version_minor = OCMD_FILE_FORMAT_VERSION_MINOR,
      // .file_version_patch = OCMD_FILE_FORMAT_VERSION_PATCH,
      .prog_header = {
        // code section always starts right after the header
        .has_code_flag = static_cast<uint8_t>(!code.compiled_blocks.empty()),
        .code_section_offset = sizeof(ocmd_file_header),
        .data_section_offset = 0,
        .data_table_offset = 0,
        .entry_point_address = 0,
        .num_instructions = 0,
      }
    };
    std::span bytes{ reinterpret_cast<const uint8_t*>(&header), sizeof(header) };
    CORE_LOG_DEBUG("[HEADER] Writing Range: [0x0000, {:#04x})", bytes.size());
    binary.append_range(bytes);
  }

  void ocmd_linker::write_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (const auto& code_block : code.compiled_blocks) {
      auto bytes_view =
        code_block.artifact.machine_instructions |
        std::views::transform([&](const instruction& instr) { return opcode_to_bytes(instr); }) |
        std::views::join |
        std::ranges::to<std::vector>();
      resolver->attach_code_label(code_block.name, binary.size());
      CORE_LOG_DEBUG("[LINK] Attaching code label '{}' @ {:#04x}", code_block.name, binary.size());
      CORE_LOG_DEBUG("[CODE] Writing Range: [{:#04x}, {:#04x})", binary.size(), binary.size() + bytes_view.size());
      binary.append_range(bytes_view);
    }
  }

  // void ocmd_linker::write_generated_code(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary, std::span<const uint8_t> generated_code) {
  //   binary.append_range(generated_code);
  // }

  void ocmd_linker::write_data_sections(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (const auto& data_section : code.compiled_data_sections) {
      // don't normalize here because using acutal size of output binary here
      uint16_t data_section_start_address = binary.size();
      resolver->attach_data_symbol(data_section.name, data_section_start_address);
      CORE_LOG_DEBUG("[LINK] Attaching data symbol '{}' @ {:#04x}", data_section.name, data_section_start_address);
      for (const auto& field : data_section.fields) {
        const std::string full_field_name = std::format("{}.{}", data_section.name, field.name);
        resolver->attach_data_symbol(full_field_name, data_section_start_address + field.offset);
        CORE_LOG_DEBUG("[LINK]          sub-symbol '{}' @ {:#04x}", full_field_name, data_section_start_address + field.offset);
      }
      CORE_LOG_DEBUG("[DATA] Writing Range: [{:#04x}, {:#04x})", binary.size(), binary.size() + data_section.data.size());
      binary.append_range(data_section.data);
    }
  }

  void ocmd_linker::do_final_linking(scope<symbol_resolver>& resolver, std::vector<uint8_t>& binary) {
    for (uint32_t code_block_index = 0; code_block_index < code.compiled_blocks.size(); ++code_block_index) {
      auto& code_block = code.compiled_blocks[code_block_index];
      uint16_t code_block_start_address = calculate_code_section_offset(code_block_index);

      // remove those we can, global linker later might remove more
      for (auto fixup_itr = code_block.artifact.unresolved_labels.begin();
           fixup_itr != code_block.artifact.unresolved_labels.end();) {
        CORE_LOG_DEBUG("[LINK] Resolving symbol '{}' for block '{}'", fixup_itr->symbol_name, code_block.name);
        auto fixup = resolver->resolve_symbol(fixup_itr->symbol_name);

        if (fixup.final_address != 0) {
          uint16_t code_section_offset = (fixup_itr->opcode_index * other_command_device::kOpCodeSize);
          uint16_t binary_address = code_block_start_address + code_section_offset;
          CORE_LOG_DEBUG("[LINK] Patching '{}' @ {:#04x} w/ {:#04x}", fixup_itr->symbol_name, binary_address, fixup.final_address);
          uint8_t* instr_pointer = binary.data() + binary_address;
          instruction& instr = *reinterpret_cast<instruction*>(instr_pointer);
          instr.lower = fixup.final_address;
          CORE_LOG_DEBUG("[LINK]         patched opcode {:#010x}", instr.opcode);
        } else {
          CORE_LOG_DEBUG("[LINK]         symbol could not be resolved yet, leaving fixup for later linking");
        }

        fixup_itr = code_block.artifact.unresolved_labels.erase(fixup_itr);
      }
    }
  }

  std::vector<uint8_t> ocmd_linker::create_compiler_generated_symbols(scope<symbol_resolver>& resolver) {
    std::vector<fixup_handle> new_fixups;

    std::vector<uint8_t> binary;
    for (auto& code_block : code.compiled_blocks) {
      for (auto& instr : code_block.artifact.unresolved_labels) {
        auto fixup = resolver->resolve_symbol(instr.symbol_name);
        /// nothing should have been resolved yet we have only written the header
        if (fixup.final_address != 0) {
          throw ocmd_linking_error(std::format("Symbol '{}' was already resolved to address {:#04x} when creating compiler generated symbols, this should not happen", instr.symbol_name, fixup.final_address));
        }

        if (!fixup.invocation_thunk.empty()) {
          /**
           * resolution of symbols that have invocation thunks works by replacing a fixup symbol with a goto instead of just patching the instruction,
           *  so insert right before the patched label, we also insert a goto to go back to the label address at the end of the invocation thunk, this is the first section so we can use local addresses
           *  and it will always be correct.
           *  This allows us to support arbitrary resolution code without needing to worry about how much space it takes up or needing multiple passes to resolve everything
           *  the resolution code will be responsible for putting the resolved address in the right place in and will be compiled completely separately
           */
          const std::string gen_name = std::format("__compiler:{}", instr.symbol_name);
          resolver->register_symbol(gen_name, {});

          // this is to point the label here during real final linking
          compiler_symbol_local_addresses.push_back({ gen_name, static_cast<uint16_t>(binary.size()) });
          binary.append_range(fixup.invocation_thunk);

          // this is to mark the new fixup for the goto
          resolver->attach_code_label(std::format("{}:goto", gen_name), binary.size());
          new_fixups.push_back({ gen_name, binary.size() });

          /// add instruction to go back to wherever the fixup inserted itself, patch this later
          instruction goto_instr = opcode_goto(0xFFFF);
          binary.append_range(opcode_to_bytes(goto_instr));
        }
      }

      code_block.artifact.unresolved_labels.append_range(new_fixups);
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

}  // namespace other