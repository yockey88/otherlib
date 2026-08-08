/**
 * \file cli/tools/dev_common.hpp
 *   Shared plumbing for the dev-workflow tools (build/test/run/install/package): shared
 *   flags, source-tree guard, attached subprocess execution, built-executable lookup.
 **/
#ifndef OTHER_CLI_TOOLS_DEV_COMMON_HPP
#define OTHER_CLI_TOOLS_DEV_COMMON_HPP

#include <string>

#include "cli/process.hpp"
#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// the flags every dev tool shares; tools layer their own flags on top
    struct dev_tool_options {
      opt<std::string> config = std::nullopt;
      bool dry_run = false;
    };

    /// tries to consume a shared dev flag at args[i] (--config/-c, --env-root, --dry-run);
    ///  nullopt = "not a shared flag", error result = the flag was malformed
    opt<tool_result> try_parse_dev_flag(tool_context& ctx, std::span<const std::string> args, size_t& i, dev_tool_options& options);

    /// the dev tools drive cmake and launch build outputs, which only makes sense
    ///  against a source tree, never an installed SDK
    tool_result require_source_tree(const tool_context& ctx, std::string_view verb);

    /// runs a child process attached to the caller's console (or prints the command
    ///  under --dry-run); the returned result carries the child's exit code
    tool_result run_attached(const tool_context& ctx, process_launch launch, bool dry_run);

    /// windows locks a running exe against write/delete but not rename; if this process's own
    ///  image sits under output_root it's renamed aside and copied back, so cmake can overwrite the original
    void sidestep_running_executable(const tool_context& ctx, const filepath& output_root, std::string_view verb, bool dry_run);

    /// finds a built executable under <root>/build/<output_dir>/<config>/ (explicit config or
    ///  build_config_probe_order()); empty path = nothing built, resolved_config gets the match
    filepath find_built_executable(const environment_paths& env, const filepath& output_dir, std::string_view executable_name,
                                   const opt<std::string>& config, std::string& resolved_config);

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_DEV_COMMON_HPP
