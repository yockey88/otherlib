/**
 * \file vm/command_files/ocmd_compiler.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include "core/enum_formatter.hpp"

#include "vm/command_files/code_block.hpp"
#include "vm/command_files/compiler_error.hpp"
#include "vm/command_files/opcode_builder.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
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

  ocmd_program ocmd_compiler::compile(scope<ocmd_code_generator> generator, diagnostic_engine* diag) {
    OTHER_ASSERT(generator != nullptr, "Code generator scope is null in ocmd_compiler::compile!");
    OTHER_ASSERT(diag != nullptr, "Diagnostic engine cannot be null");
    diagnostics = diag;

    if (ir.target_vm_version > generator->target_version()) {
      throw ocmd_toolchain_error(COMPILER_ERROR_INVALID_VERSION, std::format("Parsed IR has newer target version than code generator target version! IR target: {}, generator target: {}", ir.target_vm_version, generator->target_version()));
    }

    EMIT_TRACE("Compiling program w/ {} code blocks and {} data blocks", ir.code_blocks.size(), ir.data_blocks.size());
    ocmd_program program{
      .compiler_version = generator->target_version(),
      .definitions = ir.definitions,
    };
    program.compiled_blocks.reserve(ir.code_blocks.size());
    program.compiled_data_sections.reserve(ir.data_blocks.size());

    for (const auto& code_blk_ir : ir.code_blocks) {
      EMIT_TRACE(" - code block {}", code_blk_ir.name);
      opcode_builder builder(generator->target_version());
      for (const auto& instr_ir : code_blk_ir.instructions) {
        builder.lower_raw_instruction(instr_ir);
      }
      compiled_code_block out = {
        .name = code_blk_ir.name,
        .is_entry_point = code_blk_ir.is_entry_point,
        .artifact = builder.finalize(*generator),
      };
      program.compiled_blocks.push_back(std::move(out));
    }

    for (const auto& data_blk_ir : ir.data_blocks) {
      EMIT_TRACE(" - data block {}", data_blk_ir.name);
      compiled_data_section out = {
        .name = data_blk_ir.name,
        .fields = {},
        .data = {},
      };

      {
        size_t sz = 0;
        for (const auto& obj_ir : data_blk_ir.objects) {
          sz += obj_ir.data.size();
        }
        out.data.reserve(sz);
      }

      {
        size_t current_offset = 0;
        for (const auto& obj_ir : data_blk_ir.objects) {
          compiled_data_section::field field{
            .name = obj_ir.name,
            .offset = static_cast<uint32_t>(current_offset),
            .size = static_cast<uint32_t>(obj_ir.data.size()),
          };
          current_offset += obj_ir.data.size();
          out.data.insert(out.data.end(), obj_ir.data.begin(), obj_ir.data.end());
          out.fields.push_back(std::move(field));
        }
      }

      program.compiled_data_sections.push_back(std::move(out));
    }

    program.valid = true;
    return program;
  }

#undef TRACE_ARGS
#undef EMIT_TRACE

}  // namespace other