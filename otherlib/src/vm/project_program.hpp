/**
 * \file vm/project_program.hpp
 * a project's boot program: every VM source the project has, linked behind an entry the
 * engine writes. the entry is the caller of the hooks that belong to the project load
 **/
#ifndef OTHERLIB_VM_PROJECT_PROGRAM_HPP
#define OTHERLIB_VM_PROJECT_PROGRAM_HPP

#include <span>

#include "data-structures/std_container.hpp"

#include "vm/command_files/ocmd_program.hpp"
#include "vm/hooks.hpp"

namespace other {

  class command_bus;
  class diagnostic_engine;

  /// names that start with this belong to the generated unit; a project may not define one
  constexpr inline std::string_view kGeneratedNamePrefix = "__boot";

  /// what a project file says about its VM sources
  struct project_vm_settings {
    filepath project_file;
    /// [vm] boot-file, found the way any asset path is (the project's folder first); empty
    ///  when the project has no such key
    opt<filepath> boot_file;
    /// <project folder>/assets/vm, whether or not it exists
    filepath sources_folder;
  };

  struct project_vm_source {
    filepath path;
    /// the path from the project's folder, with forward slashes. it names the file in errors,
    ///  in the debug section, and decides its place in the link, so it is the same on every machine
    std::string name;
  };

  /// one block that handles a hook
  struct project_hook_handler {
    hook_id hook = hook_id::PRE_LOAD;
    /// the block to call
    std::string block;
    /// the source it is in
    std::string unit;
  };

  struct project_program_options {
    /// what the boot program hands to project.load: the project FILE, as the driver that runs
    ///  the program can find it from where it starts. empty means the path of the project file
    ///  as it was given to the build
    std::string project_load_path;
    ocmd_link_options link = {};
  };

  struct project_program_result {
    ostd::vector<uint8_t> bytes;
    bool success = false;
    /// the project's own sources that were linked, in link order
    ostd::vector<std::string> sources;
    /// every handler the project has for a hook. the handlers of one hook are in the order
    ///  they run in, which is the order of the files they are in
    ostd::vector<project_hook_handler> hooks;
    /// handlers for engine events. they are in the program, and nothing calls them yet
    ostd::vector<std::string> uncalled_handlers;
    /// what is likely a mistake in a hook. a strict build fails when there is one
    ostd::vector<std::string> warnings;
    /// the text of the generated unit, for a person to read
    std::string boot_unit;
  };

  /// empty when the file cannot be read or is not a project file. a project with no [vm] table is fine
  opt<project_vm_settings> read_project_vm_settings(const filepath& project_file);

  /// the boot file and every .ocmd and .oasm under assets/vm, each once, in name order.
  ///  a .obin in there is a build's output and is left out
  ostd::vector<project_vm_source> collect_project_vm_sources(const project_vm_settings& settings);

  /// the unit that holds the program's entry: the pre_load handlers, the load and the wait for
  ///  it, then the post_load handlers. the handlers of each hook are called in the order given
  std::string generate_boot_unit(std::string_view project_load_path, std::span<const project_hook_handler> hooks);

  /// links the project's sources and the generated entry into one program. commands resolve
  ///  against the devices on bus, so it must hold what the driver that runs the program registers
  project_program_result build_project_program(const command_bus& bus, const filepath& project_file, diagnostic_engine* diag, const project_program_options& options = {});

}  // namespace other

#endif  // OTHERLIB_VM_PROJECT_PROGRAM_HPP
