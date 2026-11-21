/**
 * \file vm/project_program.cpp
 **/
#include "vm/project_program.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <toml++/toml.hpp>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "file/path_helpers.hpp"

#include "vm/command_files/ocmd_toolchain.hpp"
#include "vm/command_language/command_lowering.hpp"
#include "vm/diagnostics/diagnostic_engine.hpp"
#include "vm/opcode.hpp"

namespace other {
  namespace {

    constexpr std::string_view kBootUnitName = "__boot.oasm";
    constexpr std::string_view kSourcesFolder = "assets/vm";
    constexpr std::string_view kBootFileKey = "vm.boot-file";

    opt<std::string> read_text(const filepath& path) {
      std::ifstream file(path, std::ios::binary);
      if (!file.is_open()) {
        return std::nullopt;
      }
      std::ostringstream text;
      text << file.rdbuf();
      return text.str();
    }

    bool is_vm_source(const filepath& path) {
      return path.extension() == ".ocmd" || path.extension() == ".oasm";
    }

    filepath canonical_path(const filepath& path) {
      std::error_code ec;
      const filepath canonical = std::filesystem::weakly_canonical(path, ec);
      return ec ? path : canonical;
    }

    /// the path from the project's folder; a file outside it goes by its file name alone
    std::string source_name(const filepath& path, const filepath& project_folder) {
      std::error_code ec;
      const filepath relative = std::filesystem::relative(path, project_folder, ec);
      const bool inside = !ec && !relative.empty() && *relative.begin() != "..";
      return inside ? relative.generic_string() : path.filename().generic_string();
    }

    bool is_reserved(const std::string_view name) {
      return name.starts_with(kGeneratedNamePrefix);
    }

    bool is_stop(const instruction& instr) {
      return instr.opcode == opcode_stop_device();
    }

  }  // namespace

  opt<project_vm_settings> read_project_vm_settings(const filepath& project_file) {
    PROFILE_SECTION("read_project_vm_settings");
    const opt<std::string> text = read_text(project_file);
    if (!text.has_value()) {
      CORE_LOG_ERROR("[VM] : project file '{}' does not exist or cannot be read", project_file.string());
      return std::nullopt;
    }

    toml::table table;
    try {
      table = toml::parse(*text);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("[VM] : '{}' is not a project file: {}", project_file.string(), e.what());
      return std::nullopt;
    }

    const filepath folder = project_file.parent_path();
    project_vm_settings settings = { .project_file = project_file, .sources_folder = folder / kSourcesFolder };
    if (const opt<std::string> boot_file = table.at_path(kBootFileKey).value<std::string>(); boot_file.has_value() && !boot_file->empty()) {
      /// left under the project's folder when it is not found, so the error says where it was looked for
      const asset_path found = resolve_asset_path(*boot_file, project_file);
      settings.boot_file = found.found() ? found.resolved : folder / *boot_file;
    }
    return settings;
  }

  ostd::vector<project_vm_source> collect_project_vm_sources(const project_vm_settings& settings) {
    PROFILE_SECTION("collect_project_vm_sources");
    const filepath project_folder = canonical_path(settings.project_file.parent_path());

    ostd::vector<project_vm_source> sources;
    const auto add = [&sources, &project_folder](const filepath& path) {
      const filepath canonical = canonical_path(path);
      if (std::ranges::find(sources, canonical, &project_vm_source::path) == sources.end()) {
        sources.push_back({ .path = canonical, .name = source_name(canonical, project_folder) });
      }
    };

    /// in a packaged project the key names the linked .obin, which is not a source
    if (settings.boot_file.has_value() && is_vm_source(*settings.boot_file)) {
      add(*settings.boot_file);
    }

    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator entry(settings.sources_folder, ec), end; !ec && entry != end; entry.increment(ec)) {
      if (entry->is_regular_file(ec) && is_vm_source(entry->path())) {
        add(entry->path());
      }
    }

    /// name order, so the same project gives the same program however the folder is listed
    std::ranges::sort(sources, {}, &project_vm_source::name);
    return sources;
  }

  /// every hook there is today is called from the generated entry. a new row in the hook table needs
  ///  its caller written before this number goes up, or its handlers are linked and never run
  static_assert(static_cast<size_t>(hook_id::NUM_HOOKS) == 2, "decide what calls the new hook");

  std::string generate_boot_unit(const std::string_view project_load_path, std::span<const project_hook_handler> hooks) {
    const std::string path{ project_load_path };
    const std::string refused = std::format("the project did not start loading: {}", path);

    std::string text = "; generated: the entry of a project's boot program\n/entry: __boot_main\n\n";
    text += "#__boot_project {\n";
    text += std::format("{}\n", command_lowering::text_field("path", path));
    text += std::format("  .path_length : = {}\n", path.size());
    text += std::format("{}\n", command_lowering::text_field("refused", refused));
    text += std::format("  .refused_length : = {}\n", refused.size());
    text += "}\n\n";

    const auto call_handlers = [&text, hooks](const hook_id hook) {
      for (const project_hook_handler& handler : hooks) {
        if (handler.hook == hook) {
          text += std::format("  call {}    ; {}\n", handler.block, handler.unit);
        }
      }
    };

    text += "$__boot_main:\n";
    call_handlers(hook_id::PRE_LOAD);
    /// a hook may leave anything in rflag, and the devices read it as well as write it, so it
    ///  is cleared before each syscall. project.load leaves it set when nothing started loading
    text += "  set r0, __boot_project.path\n";
    text += "  set r1, [__boot_project.path_length]\n";
    text += "  set rflag, 0\n";
    text += "  syscall project.load\n";
    text += "  jne __boot_refused\n";
    /// the load runs over many frames; the device spends its share of each one asking
    text += "@__boot_wait:\n";
    text += "  set rflag, 0\n";
    text += "  syscall project.loaded\n";
    text += "  mov rf, rflag\n";
    text += "  je __boot_wait\n";
    call_handlers(hook_id::POST_LOAD);
    text += "  stopdev\n";
    /// level 4 is an error. the device then stops with the project still empty, which is what
    ///  a driver takes as a failed boot
    text += "@__boot_refused:\n";
    text += "  set r0, __boot_project.refused\n";
    text += "  set r1, [__boot_project.refused_length]\n";
    text += "  set r2, 4\n";
    text += "  set rflag, 0\n";
    text += "  syscall core.log\n";
    text += "  stopdev\n";
    text += "end\n";
    return text;
  }

  project_program_result build_project_program(const command_bus& bus, const filepath& project_file, diagnostic_engine* diag, const project_program_options& options) {
    OTHER_ASSERT(diag != nullptr, "Diagnostic engine cannot be null");
    PROFILE_SECTION("build_project_program");
    project_program_result result;

    const opt<project_vm_settings> settings = read_project_vm_settings(project_file);
    if (!settings.has_value()) {
      return result;
    }
    if (settings->boot_file.has_value() && !std::filesystem::exists(*settings->boot_file)) {
      CORE_LOG_ERROR("[VM] : the boot file the project names, '{}', does not exist", settings->boot_file->string());
      return result;
    }

    /// hand-written assembly goes to the compiler as it is; scripts are lowered to assembly first
    ocmd_toolchain toolchain;
    const ostd::vector<project_vm_source> sources = collect_project_vm_sources(*settings);
    ostd::vector<oasm_source> assembly;
    ostd::vector<command_unit> scripts;
    for (const project_vm_source& source : sources) {
      const opt<std::string> text = read_text(source.path);
      if (!text.has_value()) {
        CORE_LOG_ERROR("[VM] : '{}' cannot be read", source.name);
        return result;
      }
      result.sources.push_back(source.name);
      if (source.path.extension() == ".oasm") {
        assembly.push_back({ .name = source.name, .text = *text });
        continue;
      }

      opt<command_file> file = toolchain.parse_command_source(source.name, *text, diag);
      if (!file.has_value()) {
        CORE_LOG_ERROR("[VM] : '{}' has errors and was not built", source.name);
        return result;
      }
      /// the folder decides what is in a project's build, so 'use' adds nothing; it is checked,
      ///  so a script cannot quietly depend on a file that will not be there
      for (const command_use& use : file->uses) {
        const filepath used = canonical_path(source.path.parent_path() / use.path);
        if (std::ranges::find(sources, used, &project_vm_source::path) == sources.end()) {
          CORE_LOG_ERROR("[VM] : '{}' uses '{}', which is not one of the project's VM sources", source.name, use.path);
          return result;
        }
      }
      scripts.push_back({ .name = source.name, .file = std::move(*file) });
    }

    ostd::vector<command_hook_record> handlers;
    const size_t assembly_count = assembly.size();
    if (!scripts.empty()) {
      const command_build_options build_options = { .has_entry_unit = false, .name_by_path = true, .strict = options.link.strict };
      ostd::vector<oasm_source> lowered = toolchain.lower_command_units(bus, scripts, diag, build_options, &handlers);
      if (lowered.empty()) {
        return result;
      }
      assembly.append_range(std::move(lowered));
    }

    ostd::vector<ocmd_unit> units;
    if (!assembly.empty()) {
      units = toolchain.compile_units(assembly, diag);
      if (units.empty()) {
        return result;
      }
    }

    bool takes_a_reserved_name = false;
    for (const ocmd_unit& unit : units) {
      for (const compiled_code_block& block : unit.program.compiled_blocks) {
        if (is_reserved(block.name)) {
          CORE_LOG_ERROR("[VM] : block '{}' in '{}' has a name kept for the generated boot unit; names may not start with '{}'", block.name, unit.name, kGeneratedNamePrefix);
          takes_a_reserved_name = true;
        }
      }
      for (const compiled_data_section& section : unit.program.compiled_data_sections) {
        if (is_reserved(section.name)) {
          CORE_LOG_ERROR("[VM] : data block '{}' in '{}' has a name kept for the generated boot unit; names may not start with '{}'", section.name, unit.name, kGeneratedNamePrefix);
          takes_a_reserved_name = true;
        }
      }
    }
    if (takes_a_reserved_name) {
      return result;
    }

    /// in hand-written assembly a hook is a block of the hook's name. the first assembly_count
    ///  units are those files; a script's blocks are never named this way
    for (size_t i = 0; i < assembly_count; ++i) {
      for (const compiled_code_block& block : units[i].program.compiled_blocks) {
        const opt<hook_id> hook = get_hook_id(block.name);
        if (!hook.has_value()) {
          continue;
        }
        result.hooks.push_back({ .hook = *hook, .block = block.name, .unit = units[i].name });
        /// the compiler ends every block with a ret, so a hook always can come back; one that
        ///  stops the device on the way takes with it whatever its caller had left to do
        if (std::ranges::any_of(block.artifact.machine_instructions, is_stop)) {
          result.warnings.push_back(std::format("hook '{}' in '{}' stops the device, so nothing after it runs", block.name, units[i].name));
        }
      }
    }
    for (const std::string& warning : result.warnings) {
      CORE_LOG_WARN("[VM] : {}", warning);
    }
    if (options.link.strict && !result.warnings.empty()) {
      CORE_LOG_ERROR("[VM] : the build is strict, so its warnings stop it");
      return result;
    }
    /// in a script a hook is an 'on' handler. one for an engine event is linked in, and waits
    ///  for the engine to be able to call a block by name
    for (const command_hook_record& handler : handlers) {
      const opt<hook_id> hook = handler.engine_event ? std::nullopt : get_hook_id(handler.name);
      if (hook.has_value()) {
        result.hooks.push_back({ .hook = *hook, .block = handler.block, .unit = handler.unit });
      } else {
        result.uncalled_handlers.push_back(std::format("{} in {}", handler.name, handler.unit));
      }
    }
    /// several files may each have a handler for one hook; they run in the order of their names
    std::ranges::stable_sort(result.hooks, {}, &project_hook_handler::unit);

    const std::string load_path = options.project_load_path.empty() ? project_file.generic_string() : options.project_load_path;
    result.boot_unit = generate_boot_unit(load_path, result.hooks);
    const oasm_source boot_source = { .name = std::string{ kBootUnitName }, .text = result.boot_unit };
    ostd::vector<ocmd_unit> boot_unit = toolchain.compile_units(std::span{ &boot_source, 1 }, diag);
    if (boot_unit.empty()) {
      return result;
    }
    units.append_range(std::move(boot_unit));

    obin_build_result linked = toolchain.link_units(bus, units, diag, options.link);
    result.bytes = std::move(linked.bytes);
    result.success = linked.success;
    return result;
  }

}  // namespace other
