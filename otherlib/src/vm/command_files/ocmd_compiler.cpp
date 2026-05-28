/**
 * \file vm/command_files/ocmd_compiler.cpp
 **/
#include "vm/command_files/ocmd_compiler.hpp"

#include "core/enum_formatter.hpp"

#include "vm/command_files/code_block.hpp"
#include "vm/command_files/compiler_error.hpp"
#include "vm/command_files/opcode_builder.hpp"

namespace other {

  ocmd_program ocmd_compiler::compile(scope<ocmd_code_generator> generator) {
    OTHER_ASSERT(generator != nullptr, "Code generator scope is null in ocmd_compiler::compile!");

    if (ir.target_vm_version > generator->target_version()) {
      throw ocmd_lowering_error(std::format("Parsed IR has newer target version than code generator target version! IR target: {}, generator target: {}", ir.target_vm_version, generator->target_version()));
    }

    ocmd_program program{
      .compiler_version = generator->target_version(),
      .definitions = ir.definitions,
    };
    program.compiled_blocks.reserve(ir.code_blocks.size());
    program.compiled_data_sections.reserve(ir.data_blocks.size());

    for (const auto& code_blk_ir : ir.code_blocks) {
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

}  // namespace other