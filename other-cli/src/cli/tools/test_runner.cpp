/**
 * \file cli/tools/test_runner.cpp
 **/
#include "cli/tools/test_runner.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/profiler.hpp"

#include "cli/tools/dev_common.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kTestUsage =
        R"(usage: test [options...]

  Runs the Other Environment unit test suites (gtest) or one of the harness
  scenarios (soak/network/fuzz/stress) from a source tree build.
  Build the suites first with: build --tests

  options:
    -s,  --seed <seed>        seed for gtest shuffle (default: random)
    -n,  --num-runs <count>   run the tests multiple times (default: 1)
    -c,  --config <config>    build config to test (default: first config with a test build)
    -f,  --filter <pattern>   forwarded to --gtest_filter to run a subset of suites
         --xml <path>         gtest xml report path
                              (default: other_test_results.windows.<debug|release>.xml)
    -bf, --break-on-failure   stop on first test failure (default: false)
         --no-shuffle         run tests in declaration order instead of shuffled
         --soak               run the soak harness and validate logs/soak-report.json
         --network            run the network harness scenario and validate
                              logs/network-report.json (the tag-pipeline gate)
         --fuzz               run the parser fuzz harness and validate
                              logs/fuzz-report.json (the tag-pipeline gate)
         --stress             run the frame-load stress harness and validate
                              logs/stress-report.json (the tag-pipeline gate)
         --env-root <path>    explicit source tree root
         --dry-run            print the launch instead of running it)";

      /// harness runs share one shape: launch the scenario driver, then read the verdict from its
      ///  report; the driver exits 0 for any clean run — the verdict lives in the report (the contract)
      tool_result run_harness_scenario(tool_context& ctx, const dev_tool_options& options,
                                       std::string_view label, const filepath& scenario_config,
                                       std::string_view report_name) {
        PROFILE_SECTION("run_harness_scenario");
        std::string resolved_config = "";
        const filepath harness = find_built_executable(ctx.env, filepath("tests") / "harness", "other_soak", options.config, resolved_config);
        if (harness.empty()) {
          return tool_result::error(std::format("no{} harness build found under 'build/tests/harness' (build one with: build --tests{})",
                                                options.config.has_value() ? std::format(" {}", options.config.value()) : "",
                                                options.config.has_value() ? std::format(" --config {}", options.config.value()) : ""));
        }

        const filepath report_path = ctx.env.root / "logs" / report_name;
        if (!options.dry_run) {
          std::error_code ec;
          std::filesystem::remove(report_path, ec);
        }

        ctx.print("running {} harness [{}]", label, resolved_config);
        const tool_result ran = run_attached(ctx,
                                             { .executable = harness,
                                               .arguments = { scenario_config.string() },
                                               .working_directory = ctx.env.root },
                                             options.dry_run);
        if (options.dry_run) {
          return tool_result::ok("dry run only, nothing launched");
        }
        if (!ran.success()) {
          return { .code = ran.code, .message = std::format("{} harness exited with code {}", label, ran.code) };
        }

        if (!std::filesystem::exists(report_path)) {
          return tool_result::error(std::format("{} FAILED: no report written to '{}' (harness crashed or never finalized)", label, report_path.string()));
        }

        std::ifstream report_file(report_path);
        const nlohmann::json report = nlohmann::json::parse(report_file, nullptr, false);
        if (report.is_discarded()) {
          return tool_result::error(std::format("{} FAILED: report '{}' is not valid json", label, report_path.string()));
        }

        const bool passed = report.value("pass", false);
        const std::string reason = report.value("reason", "");
        ctx.print("{} result: {} - {}", label, passed ? "PASS" : "FAIL", reason);
        if (!passed) {
          return tool_result::error(std::format("{} failed: {}", label, reason));
        }
        return tool_result::ok(std::format("{} passed", label));
      }

      tool_result run_soak_harness(tool_context& ctx, const dev_tool_options& options) {
        return run_harness_scenario(ctx, options, "soak",
                                    filepath("tests") / "harness" / "soak-config.toml", "soak-report.json");
      }

      tool_result run_network_harness(tool_context& ctx, const dev_tool_options& options) {
        return run_harness_scenario(ctx, options, "network",
                                    filepath("tests") / "harness" / "network-config.toml", "network-report.json");
      }

      tool_result run_fuzz_harness(tool_context& ctx, const dev_tool_options& options) {
        return run_harness_scenario(ctx, options, "fuzz",
                                    filepath("tests") / "harness" / "fuzz-config.toml", "fuzz-report.json");
      }

      tool_result run_stress_harness(tool_context& ctx, const dev_tool_options& options) {
        return run_harness_scenario(ctx, options, "stress",
                                    filepath("tests") / "harness" / "stress-config.toml", "stress-report.json");
      }

    }  // namespace

    std::string_view test_runner_tool::usage() const {
      return kTestUsage;
    }

    tool_result test_runner_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("test_runner_tool::execute");
      dev_tool_options options;
      opt<std::string> filter = std::nullopt;
      opt<std::string> xml_path = std::nullopt;
      uint32_t num_runs = 1;
      opt<uint32_t> seed = std::nullopt;
      bool shuffle = true;
      bool break_on_failure = false;
      bool soak = false;
      bool network = false;
      bool fuzz = false;
      bool stress = false;

      for (size_t i = 0; i < args.size(); ++i) {
        if (opt<tool_result> shared = try_parse_dev_flag(ctx, args, i, options); shared.has_value()) {
          if (!shared.value().success()) {
            return shared.value();
          }
          continue;
        }

        const std::string& arg = args[i];
        if (arg == "-s" || arg == "--seed") {
          if (i + 1 >= args.size()) {
            return tool_result::error(std::format("'{}' requires a seed value", arg));
          }
          seed = std::stoul(args[++i]);
        } else if (arg == "--filter" || arg == "-f") {
          if (i + 1 >= args.size()) {
            return tool_result::error(std::format("'{}' requires a gtest filter pattern", arg));
          }
          filter = args[++i];

        } else if (arg == "--num-runs" || arg == "-n") {
          if (i + 1 >= args.size()) {
            return tool_result::error(std::format("'{}' requires a run count", arg));
          }
          num_runs = std::stoul(args[++i]);
        } else if (arg == "--xml") {
          if (i + 1 >= args.size()) {
            return tool_result::error("'--xml' requires a report path");
          }
          xml_path = args[++i];
        } else if (arg == "--no-shuffle") {
          shuffle = false;
        } else if (arg == "-bf" || arg == "--break-on-failure") {
          break_on_failure = true;
        } else if (arg == "--soak") {
          soak = true;
        } else if (arg == "--network") {
          network = true;
        } else if (arg == "--fuzz") {
          fuzz = true;
        } else if (arg == "--stress") {
          stress = true;
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
      if (network) {
        return run_network_harness(ctx, options);
      }
      if (fuzz) {
        return run_fuzz_harness(ctx, options);
      }
      if (stress) {
        return run_stress_harness(ctx, options);
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
      if (seed.has_value()) {
        test_args.push_back(std::format("--gtest_random_seed={}", seed.value()));
      }
      if (filter.has_value()) {
        test_args.push_back(std::format("--gtest_filter={}", filter.value()));
      }
      if (shuffle) {
        test_args.push_back("--gtest_shuffle");
      }
      if (break_on_failure) {
        test_args.push_back("--gtest_break_on_failure");
      }
      test_args.push_back(std::format("--gtest_repeat={}", num_runs));
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
