/**
 * \file cli/process.hpp
 **/
#ifndef OTHER_CLI_PROCESS_HPP
#define OTHER_CLI_PROCESS_HPP

#include <string>
#include <vector>

#include "core/defines.hpp"

namespace other {
  namespace cli {

    struct process_launch {
      filepath executable = "";
      std::vector<std::string> arguments = {};
      filepath working_directory = "";

      bool wait_for_exit = false;
      /// detached children get their own console window so their logs outlive the cli
      bool new_console = true;
    };

    struct process_result {
      bool started = false;
      int32_t exit_code = 0;  // meaningful only when wait_for_exit was set
      std::string error = "";
    };

    /// quoted, human-readable form of the launch for logs and dry runs
    std::string format_command_line(const process_launch& launch);

    process_result launch_process(const process_launch& launch);

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_PROCESS_HPP
