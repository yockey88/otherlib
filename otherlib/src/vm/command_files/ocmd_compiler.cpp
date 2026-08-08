/**
 * \file vm/command_files/ocmd_compiler.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include <ranges>

#include "core/enum_formatter.hpp"
#include "core/profiler.hpp"

#include "vm/command_files/code_block.hpp"
#include "vm/command_files/compiler_error.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/opcode_builder.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/lexer_error_sink.hpp"
#include "vm/diagnostics/ocmd_errors.hpp"
#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

#define TRACE_ARGS(...) __VA_OPT__(, ##__VA_ARGS__)
#define EMIT_TRACE(msg, ...)                                    \
  {                                                             \
    diagnostic d = {                                            \
      .severity = VM_DIAGNOSTIC_TRACE,                          \
      .error_code = COMPILER_TRACE,                             \
      .phase = VM_PHASE_COMPILER,                               \
      .span = {},                                               \
      .final_message = std::format(msg TRACE_ARGS(__VA_ARGS__)) \
    };                                                          \
    diagnostics->emit(d);                                       \
  }

  ocmd_program ocmd_compiler::compile(const std::string_view source, scope<ocmd_code_generator> generator, diagnostic_engine* diag) {
    OTHER_ASSERT(generator != nullptr, "Code generator scope is null in ocmd_compiler::compile!");
    OTHER_ASSERT(diag != nullptr, "Diagnostic engine cannot be null");
    PROFILE_SECTION("ocmd_compiler::compile");
    diagnostics = diag;
    {
      const auto tokens = ocmd_lexer{ source }.tokenize(diag);
      if (tokens.empty()) {
        throw ocmd_toolchain_error(COMPILER_ERROR_INVALID_TOKENS, "No tokens generated from source!");
      }

      const auto ir = oasm_parser{ version, tokens }.parse(diag);
      if (!ir.valid) {
        throw ocmd_toolchain_error(COMPILER_ERROR_INVALID_IR, "IR is not valid for compilation!");
      }
      this->ir = ir;
    }
    return compile(std::move(generator), diag);
  }

  ocmd_program ocmd_compiler::compile(scope<ocmd_code_generator> generator, diagnostic_engine* diag) {
    OTHER_ASSERT(generator != nullptr, "Code generator scope is null in ocmd_compiler::compile!");
    OTHER_ASSERT(diag != nullptr, "Diagnostic engine cannot be null");
    PROFILE_SECTION("ocmd_compiler::compile--codegen");
    diagnostics = diag;

    if (!ir.valid) {
      throw ocmd_toolchain_error(COMPILER_ERROR_INVALID_IR, "IR is not valid for compilation!");
    }
    if (ir.target_vm_version > generator->target_version()) {
      throw ocmd_toolchain_error(COMPILER_ERROR_INVALID_VERSION, std::format("Parsed IR has newer target version than code generator target version! IR target: {}, generator target: {}", ir.target_vm_version, generator->target_version()));
    }

    EMIT_TRACE("Compiling program w/ {} code blocks and {} data blocks", ir.code_blocks.size(), ir.data_blocks.size());
    EMIT_TRACE(" - target VM version: {}", ir.target_vm_version);
    // loose instructions outside any named block collect into one "__natural_entry" block,
    // used as the default entry point when no /entry directive is set; always created
    {
      code_block native_entry = {
        .name = "__natural_entry",
      };
      native_entry.name_hash = FNV(native_entry.name);
      for (const auto& code_blk_ir : ir.code_blocks | std::views::filter([](const auto& blk) { return blk.name == "__natural_entry"; })) {
        EMIT_TRACE(" - found __natural_entry block with {} instructions, moving to main natural entry", code_blk_ir.instructions.size());
        native_entry.instructions.insert(native_entry.instructions.end(), code_blk_ir.instructions.begin(), code_blk_ir.instructions.end());
        native_entry.jump_labels.insert(native_entry.jump_labels.end(), code_blk_ir.jump_labels.begin(), code_blk_ir.jump_labels.end());
      }
      while (true) {
        auto it = std::ranges::find_if(ir.code_blocks, [](const auto& blk) { return blk.name == "__natural_entry"; });
        if (it == ir.code_blocks.end()) {
          break;
        }
        ir.code_blocks.erase(it);
      }
      ir.code_blocks.push_back(std::move(native_entry));
    }
    EMIT_TRACE(" - __natural_entry blocks merged, total code blocks: {}", ir.code_blocks.size());

    /// resolve any definitions if possible

    ocmd_program program{
      .compiler_version = generator->target_version(),
      .definitions = ir.definitions,
    };
    program.compiled_blocks.reserve(ir.code_blocks.size());
    program.compiled_data_sections.reserve(ir.data_blocks.size());

    if (auto entry_def_itr = std::ranges::find_if(program.definitions, [](const auto& def) { return def.name == "entry"; });
        entry_def_itr != program.definitions.end()) {
      auto code_block_itr = std::ranges::find_if(ir.code_blocks, [&](const auto& blk) { return blk.name == entry_def_itr->value.text; });
      if (code_block_itr == ir.code_blocks.end()) {
        EMIT_TRACE(" - entry symbol {} not found", entry_def_itr->value.text);
        program.definitions.erase(entry_def_itr);
      }
      // insert a stopdev at the end of the entry function
      else if (code_block_itr->instructions.empty() || code_block_itr->instructions.back().opcode != canonical_opcode::STOPDEV_OP) {
        EMIT_TRACE(" - entry symbol {} resolved to code block {}", entry_def_itr->value.text, code_block_itr->name);

        /// if entry point ends in ret, remove it and stop the device instead
        /// \todo fix once we link multiple source files
        if (code_block_itr->instructions.back().opcode == canonical_opcode::RET_OP) {
          code_block_itr->instructions.erase(code_block_itr->instructions.end() - 1);
          code_block_itr->instructions.push_back({ .opcode = canonical_opcode::STOPDEV_OP });
        }

        code_block_itr->instructions.push_back({ .opcode = canonical_opcode::STOPDEV_OP });
      }
    }

    for (const auto& code_blk_ir : ir.code_blocks) {
      PROFILE_SECTION("ocmd_compiler::compile--lower_code_block");
      EMIT_TRACE(" - code block {}", code_blk_ir.name);
      opcode_builder builder(generator->target_version());
      for (const auto& instr_ir : code_blk_ir.instructions) {
        builder.lower_raw_instruction(instr_ir);
      }
      for (const auto& jump_label_ir : code_blk_ir.jump_labels) {
        EMIT_TRACE("   - jump label {} w/ index = {}", jump_label_ir.name, jump_label_ir.instruction_index);
        builder.add_jump_label(jump_label_ir.name, jump_label_ir.instruction_index);
      }
      compiled_code_block out = {
        .name = code_blk_ir.name,
        .artifact = builder.finalize(*generator),
      };
      program.compiled_blocks.push_back(std::move(out));
    }

    for (const auto& data_blk_ir : ir.data_blocks) {
      PROFILE_SECTION("ocmd_compiler::compile--pack_data_section");
      EMIT_TRACE(" - data block {}", data_blk_ir.name);
      compiled_data_section out = {
        .name = data_blk_ir.name,
        .fields = {},
        .data = {},
      };

      {
        size_t current_offset = 0;
        for (const auto& obj_ir : data_blk_ir.objects) {
          compiled_data_section::field field{
            .name = obj_ir.name,
            .offset = static_cast<uint32_t>(current_offset),
            .size = static_cast<uint32_t>(obj_ir.data.size()),
          };

          size_t data_size = obj_ir.data.size();
          size_t padding_needed = 0;
          if (data_size % kDefaultAlignment != 0) {
            padding_needed = kDefaultAlignment - (data_size % kDefaultAlignment);
          }
          data_size += padding_needed;

          out.data.insert(out.data.end(), obj_ir.data.begin(), obj_ir.data.end());
          for (size_t i = 0; i < padding_needed; i++) {
            out.data.push_back(0x00);
          }
          out.fields.push_back(std::move(field));

          current_offset += data_size;
        }
      }

      program.compiled_data_sections.push_back(std::move(out));
    }

    /// resolve any definitions if possible

    program.valid = true;
    return program;
  }

#undef TRACE_ARGS
#undef EMIT_TRACE

}  // namespace other