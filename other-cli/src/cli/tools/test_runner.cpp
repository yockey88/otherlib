/**
 * \file cli/tools/test_runner.cpp
 **/
#include "cli/tools/test_runner.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "cli/tools/dev_common.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kTestUsage =
        R"(usage: test [options...]

  Runs the Other Environment unit test suites (gtest) or the soak harness from a
  source tree build. Build the suites first with: build --tests

  options:
    -c, --config <config>    build config to test (default: first config with a test build)
    -f, --filter <pattern>   forwarded to --gtest_filter to run a subset of suites
        --xml <path>         gtest xml report path
                             (default: other_test_results.windows.<debug|release>.xml)
        --no-shuffle         run tests in declaration order instead of shuffled
        --soak               run the soak harness and validate logs/soak-report.json
        --env-root <path>    explicit source tree root
        --dry-run            print the launch instead of running it)";

      tool_result run_soak_harness(tool_context& ctx, const dev_tool_options& options) {
        std::string resolved_config = "";
        const filepath harness = find_built_executable(ctx.env, filepath("tests") / "harness", "other_soak", options.config, resolved_config);
        if (harness.empty()) {
          return tool_result::error(std::format("no{} soak harness build found under 'build/tests/harness' (build one with: build --tests{})",
            options.config.has_value() ? std::format(" {}", options.config.value()) : "",
            options.config.has_value() ? std::format(" --config {}", options.config.value()) : ""));
        }

        const filepath report_path = ctx.env.root / "logs" / "soak-report.json";
        if (!options.dry_run) {
          std::error_code ec;
          std::filesystem::remove(report_path, ec);
        }

        ctx.print("running soak harness [{}]", resolved_config);
        const tool_result ran = run_attached(ctx,
          { .executable = harness,
            .arguments = { (filepath("tests") / "harness" / "soak-config.toml").string() },
            .working_directory = ctx.env.root },
          options.dry_run);
        if (options.dry_run) {
          return tool_result::ok("dry run only, nothing launched");
        }
        if (!ran.success()) {
          return { .code = ran.code, .message = std::format("soak harness exited with code {}", ran.code) };
        }

        /// the driver exits 0 for any clean run; the verdict lives in the report
        if (!std::filesystem::exists(report_path)) {
          return tool_result::error(std::format("soak FAILED: no report written to '{}' (harness crashed or never finalized)", report_path.string()));
        }

        std::ifstream report_file(report_path);
        const nlohmann::json report = nlohmann::json::parse(report_file, nullptr, false);
        if (report.is_discarded()) {
          return tool_result::error(std::format("soak FAILED: report '{}' is not valid json", report_path.string()));
        }

        const bool passed = report.value("pass", false);
        const std::string reason = report.value("reason", "");
        ctx.print("soak result: {} - {}", passed ? "PASS" : "FAIL", reason);
        if (!passed) {
          return tool_result::error(std::format("soak failed: {}", reason));
        }
        return tool_result::ok("soak passed");
      }

    }  // namespace

    std::string_view test_runner_tool::usage() const {
      return kTestUsage;
    }

    tool_result test_runner_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      dev_tool_options options;
      opt<std::string> filter = std::nullopt;
      opt<std::string> xml_path = std::nullopt;
      bool shuffle = true;
      bool soak = false;

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "--filter" || arg == "-f") {
          if (i + 1 >= args.size()) {
            return tool_result::error(std::format("'{}' requires a gtest filter pattern", arg));
          }
          filter = args[++i];
        } else if (arg == "--xml") {
          if (i + 1 >= args.size()) {
            return tool_result::error("'--xml' requires a report path");
          }
          xml_path = args[++i];
        } else if (arg == "--no-shuffle") {
          shuffle = false;
        } else if (arg == "--soak") {
          soak = true;
        } else {
          return tool_result::error(std::format("unknown argument '{}'\n{}", arg, kTestUsage));
        }
      }

      if (tool_result guard = require_source_tree(ctx, "run tests"); !guard.success()) {
        return guard;
      }

      if (soak) {
        return run_soak_harness(ctx, options);
      }

      std::string resolved_config = "";
      const filepath tests = find_built_executable(ctx.env, "tests", "other_tests", options.config, resolved_config);
      if (tests.empty()) {
        return tool_result::error(std::format("no{} test build found under 'build/tests' (build one with: build --tests{})",
          options.config.has_value() ? std::format(" {}", options.config.value()) : "",
          options.config.has_value() ? std::format(" --config {}", options.config.value()) : ""));
      }

      /// TODO: fix platform specific report paths when a second platform exists
      const bool debug_family = (resolved_config == "Debug" || resolved_config == "ProfileD");
      const std::string report = xml_path.value_or(
        std::format("other_test_results.windows.{}.xml", debug_family ? "debug" : "release"));

      std::vector<std::string> test_args = { (filepath("resources") / "dev-test-config.toml").string() };
      if (filter.has_value()) {
        test_args.push_back(std::format("--gtest_filter={}", filter.value()));
      }
      if (shuffle) {
        test_args.push_back("--gtest_shuffle");
      }
      test_args.push_back(std::format("--gtest_output=xml:{}", report));

      ctx.print("running test suites [{}]", resolved_config);
      const tool_result ran = run_attached(ctx,
        { .executable = tests, .arguments = test_args, .working_directory = ctx.env.root }, options.dry_run);
      if (options.dry_run) {
        return tool_result::ok("dry run only, nothing launched");
      }
      if (!ran.success()) {
        return { .code = ran.code, .message = std::format("test suites failed (exit {}), full results in '{}'", ran.code, report) };
      }
      return tool_result::ok(std::format("all test suites passed, results in '{}'", report));
    }

  }  // namespace cli
}  // namespace other
