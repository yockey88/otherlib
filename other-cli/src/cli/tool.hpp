/**
 * \file cli/tool.hpp
 **/
#ifndef OTHER_CLI_TOOL_HPP
#define OTHER_CLI_TOOL_HPP

#include <format>
#include <functional>
#include <span>
#include <string>
#include <string_view>

#include "core/defines.hpp"

#include "cli/environment.hpp"

namespace other {
  namespace cli {

    struct tool_result {
      int32_t code = 0;
      std::string message = "";

      inline bool success() const { return code == 0; }

      static inline tool_result ok(std::string message = "") {
        return { .code = 0, .message = std::move(message) };
      }
      static inline tool_result error(std::string message) {
        return { .code = 1, .message = std::move(message) };
      }
    };

    /// everything a tool needs to run, injected by the frontend so a tool behaves identically
    ///  whether invoked from the oecli executable or from inside a running environment
    struct tool_context {
      filepath working_directory = "";
      environment_paths env = {};

      std::function<void(std::string_view)> out = [](std::string_view) {};
      std::function<void(std::string_view)> err = [](std::string_view) {};

      template <typename... Args>
      void print(std::format_string<Args...> fmt, Args&&... args) const {
        out(std::format(fmt, std::forward<Args>(args)...));
      }

      template <typename... Args>
      void print_error(std::format_string<Args...> fmt, Args&&... args) const {
        err(std::format(fmt, std::forward<Args>(args)...));
      }
    };

    /// a single cli tool (create, open, ...); reports user mistakes via tool_result and never
    ///  asserts/terminates on bad input, since in-environment hosts (editor console, drivers) must survive it
    class tool {
     public:
      tool() = default;
      virtual ~tool() = default;

      virtual std::string_view name() const = 0;
      virtual std::string_view summary() const = 0;
      virtual std::string_view usage() const = 0;

      virtual tool_result execute(tool_context& ctx, std::span<const std::string> args) = 0;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOL_HPP
