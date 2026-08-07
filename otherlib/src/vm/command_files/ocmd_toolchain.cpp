/**
 * \file vm/command_files/ocmd_toolchain.cpp
 **/
#include "vm/command_files/ocmd_toolchain.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "vm/code_generator_000.hpp"
#include "vm/command_bus.hpp"
#include "vm/command_files/lexer.hpp"
#include "vm/command_files/oasm_parser.hpp"
#include "vm/command_files/ocmd_compiler.hpp"
#include "vm/command_files/ocmd_linker.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/diagnostics/ocmd_trace_sink.hpp"
#include "vm/other_device.hpp"

namespace other {

  ostd::vector<uint8_t> ocmd_toolchain::assemble_oasm_source(other_command_device* device, const filepath& path) {
    OTHER_ASSERT(std::filesystem::exists(path), "File does not exist: {}", path.string());
    OTHER_ASSERT(path.extension() == ".oasm", "File is not an OASM source file: {}", path.string());
    PROFILE_SECTION("ocmd_toolchain::assemble_oasm_source");

    std::string contents;
    {
      PROFILE_SECTION("ocmd_toolchain::assemble_oasm_source--read_source");
      std::ifstream file(path.string());
      OTHER_ASSERT(file, "Failed to open file: {}", path.string());

      std::ostringstream ss;
      ss << file.rdbuf();
      contents = ss.str();
    }

    ostd::vector<uint8_t> bytecode;
    if (contents.empty()) {
      return bytecode;
    }

    diagnostic_engine diagnostics;
    ocmd_trace_sink trace_sink;
    natural_t trace_id = diagnostics.register_sink("trace-sink", &trace_sink);

    try {
      auto program = ocmd_compiler{}.compile(contents, make_scope<code_generator_000>(), &diagnostics);
      bytecode = ocmd_linker{ program }.link(device->bus->create_default_symbol_resolver(), &diagnostics);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception caught during assembly: {}", e.what());
      bytecode = {};
    } catch (...) {
      CORE_LOG_ERROR("Unknown exception caught during assembly");
      bytecode = {};
    }

    diagnostics.remove_sink(trace_id);
    return bytecode;
  }

}  // namespace other