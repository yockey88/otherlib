/**
 * \file cli/tools/test_runner.hpp
 **/
#ifndef OTHER_CLI_TOOLS_TEST_RUNNER_HPP
#define OTHER_CLI_TOOLS_TEST_RUNNER_HPP

#include "cli/tool.hpp"

namespace other {
  namespace cli {

    /// runs the gtest unit suites (with filter and xml report support) or the soak
    ///  harness, whose verdict lives in logs/soak-report.json rather than its exit code
    class test_runner_tool final : public tool {
     public:
      test_runner_tool() = default;
      ~test_runner_tool() override = default;

      std::string_view name() const override { return "test"; }
      std::string_view summary() const override { return "Run the Other Environment test suites and soak harness"; }
      std::string_view usage() const override;

      tool_result execute(tool_context& ctx, std::span<const std::string> args) override;
    };

  }  // namespace cli
}  // namespace other

#endif  // OTHER_CLI_TOOLS_TEST_RUNNER_HPP
