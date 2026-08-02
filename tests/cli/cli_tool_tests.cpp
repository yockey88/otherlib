/**
 * \file tests/cli/cli_tool_tests.cpp
 *   Exercises the other-cli tool library end to end against temporary directories and a
 *   fake environment root; nothing here touches the real environment or spawns processes.
 **/
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <toml++/toml.hpp>

#include "cli/tool_registry.hpp"

namespace {

  using other::filepath;
  using other::cli::default_tool_registry;
  using other::cli::environment_paths;
  using other::cli::locate_environment;
  using other::cli::split_command_line;
  using other::cli::tool_context;
  using other::cli::tool_result;

  class cli_tool_tests : public ::testing::Test {
   protected:
    void SetUp() override {
      const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
      sandbox = std::filesystem::temp_directory_path() / "other-cli-tests" / info->name();
      std::filesystem::remove_all(sandbox);
      std::filesystem::create_directories(sandbox);

      ctx.working_directory = sandbox;
      ctx.out = [this](std::string_view message) { captured_out += std::format("{}\n", message); };
      ctx.err = [this](std::string_view message) { captured_err += std::format("{}\n", message); };
    }

    void TearDown() override {
      std::error_code ec;
      std::filesystem::remove_all(sandbox, ec);
    }

    /// lays down the marker files locate_environment() checks for a source tree, plus a
    ///  fake editor build for the configs given
    filepath make_fake_environment_root(const std::vector<std::string>& editor_configs = {}) {
      const filepath root = sandbox / "fake-env";
      std::filesystem::create_directories(root / "otherlib");
      std::filesystem::create_directories(root / "resources");
      std::filesystem::create_directories(root / "cmake");
      write_file(root / "resources" / "editor-config.toml", "[application]\n");
      write_file(root / "cmake" / "other_driver.cmake", "## marker\n");
      for (const std::string& config : editor_configs) {
        const filepath bin_dir = root / "build" / "other-editor" / config;
        std::filesystem::create_directories(bin_dir);
        write_file(bin_dir / "other_editor.exe", "stub");
      }
      return root;
    }

    static void write_file(const filepath& path, std::string_view contents) {
      std::ofstream file(path, std::ios::binary);
      ASSERT_TRUE(file.is_open()) << "failed to create " << path.string();
      file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    }

    tool_result execute(std::string_view line) {
      return default_tool_registry().execute_line(ctx, line);
    }

    filepath sandbox;
    tool_context ctx;
    std::string captured_out = "";
    std::string captured_err = "";
  };

  TEST(cli_command_line_tests, split_handles_quotes_and_whitespace) {
    const std::vector<std::string> expected = { "create", "my game", "--dir", "C:/a b/c" };
    EXPECT_EQ(split_command_line("  create \"my game\"   --dir 'C:/a b/c'  "), expected);
    EXPECT_TRUE(split_command_line("").empty());
    EXPECT_TRUE(split_command_line("   \t  ").empty());
    EXPECT_EQ(split_command_line("open"), std::vector<std::string>{ "open" });
  }

  TEST(cli_registry_tests, builtin_tools_are_registered) {
    auto& registry = default_tool_registry();
    EXPECT_NE(registry.find_tool("create"), nullptr);
    EXPECT_NE(registry.find_tool("open"), nullptr);
    EXPECT_EQ(registry.find_tool("does-not-exist"), nullptr);
  }

  TEST(cli_registry_tests, unknown_tool_reports_error) {
    tool_context ctx;
    const tool_result result = default_tool_registry().execute_line(ctx, "frobnicate now");
    EXPECT_FALSE(result.success());
    EXPECT_NE(result.message.find("unknown tool 'frobnicate'"), std::string::npos);

    const tool_result empty = default_tool_registry().execute_line(ctx, "   ");
    EXPECT_FALSE(empty.success());
  }

  TEST_F(cli_tool_tests, create_generates_project_without_environment) {
    const tool_result result = execute("create test-proj");
    ASSERT_TRUE(result.success()) << result.message;

    const filepath project_dir = sandbox / "test-proj";
    const filepath project_file = project_dir / "test-proj.toml";
    EXPECT_TRUE(std::filesystem::exists(project_file));
    EXPECT_TRUE(std::filesystem::exists(project_dir / "test-proj.lua"));
    EXPECT_TRUE(std::filesystem::exists(project_dir / "assets" / "scenes" / "main.lua"));
    EXPECT_TRUE(std::filesystem::is_directory(project_dir / "src"));

    toml::table table;
    ASSERT_NO_THROW(table = toml::parse_file(project_file.string()));
    EXPECT_EQ(table.at_path("scripting.projectrc-path").value_or<std::string>(""), "test-proj/test-proj.lua");
    EXPECT_EQ(table.at_path("scene-graph.starting-scene").value_or<std::string>(""), "main");
    /// no environment -> no C# project reference and no csproj on disk
    EXPECT_FALSE(table.at_path("scripting.cs_project"));
    EXPECT_FALSE(std::filesystem::exists(project_dir / "TestProj.csproj"));

    const auto* metadata = table.at_path("project.metadata").as_array();
    ASSERT_NE(metadata, nullptr);
    bool found_name = false;
    for (const auto& entry : *metadata) {
      const auto* entry_table = entry.as_table();
      ASSERT_NE(entry_table, nullptr);
      if (entry_table->at_path("key").value_or<std::string>("") == "name") {
        EXPECT_EQ(entry_table->at_path("value").value_or<std::string>(""), "test-proj");
        found_name = true;
      }
    }
    EXPECT_TRUE(found_name);
  }

  TEST_F(cli_tool_tests, create_wires_csproj_when_environment_found) {
    const filepath root = make_fake_environment_root();
    ctx.env = locate_environment(root);
    ASSERT_TRUE(ctx.env.found);
    ASSERT_TRUE(ctx.env.in_source_tree);

    const tool_result result = execute("create space-game --author Tester --description \"a demo\"");
    ASSERT_TRUE(result.success()) << result.message;

    const filepath project_dir = sandbox / "space-game";
    const filepath csproj = project_dir / "SpaceGame.csproj";
    ASSERT_TRUE(std::filesystem::exists(csproj));

    toml::table table;
    ASSERT_NO_THROW(table = toml::parse_file((project_dir / "space-game.toml").string()));
    EXPECT_EQ(table.at_path("scripting.cs_project").value_or<std::string>(""), "space-game/SpaceGame.csproj");

    std::ifstream file(csproj, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_NE(contents.find("OtherCs.dll"), std::string::npos);
    EXPECT_NE(contents.find(ctx.env.othercs_assembly("Debug").string()), std::string::npos);
    EXPECT_NE(contents.find(ctx.env.othercs_assembly("Release").string()), std::string::npos);
  }

  TEST_F(cli_tool_tests, create_rejects_existing_directory_and_bad_names) {
    std::filesystem::create_directories(sandbox / "taken");
    const tool_result collision = execute("create taken");
    EXPECT_FALSE(collision.success());
    EXPECT_NE(collision.message.find("already exists"), std::string::npos);

    EXPECT_FALSE(execute("create 9lives").success());
    EXPECT_FALSE(execute("create 'bad name'").success());
    EXPECT_FALSE(execute("create").success());
    EXPECT_FALSE(execute("create ok-name --bogus-flag").success());
  }

  TEST_F(cli_tool_tests, open_dry_run_resolves_project_and_editor) {
    const filepath root = make_fake_environment_root({ "Debug" });
    ctx.env = locate_environment(root);
    ASSERT_TRUE(ctx.env.found);
    ASSERT_TRUE(execute("create my-proj").success());

    const filepath project_file = sandbox / "my-proj" / "my-proj.toml";
    const tool_result by_file = execute(std::format("open \"{}\" --dry-run --config Debug", project_file.string()));
    ASSERT_TRUE(by_file.success()) << by_file.message;
    EXPECT_NE(captured_out.find("other_editor"), std::string::npos);
    EXPECT_NE(captured_out.find("-f"), std::string::npos);
    EXPECT_NE(captured_out.find(project_file.string()), std::string::npos);

    /// resolving through the containing directory finds the same project file
    captured_out.clear();
    const tool_result by_dir = execute("open my-proj --dry-run");
    ASSERT_TRUE(by_dir.success()) << by_dir.message;
    EXPECT_NE(captured_out.find(project_file.string()), std::string::npos);
  }

  TEST_F(cli_tool_tests, open_reports_missing_projects_and_builds) {
    const filepath root = make_fake_environment_root({ "Debug" });
    ctx.env = locate_environment(root);

    const tool_result nothing = execute("open --dry-run");
    EXPECT_FALSE(nothing.success());
    EXPECT_NE(nothing.message.find("no project file found"), std::string::npos);

    ASSERT_TRUE(execute("create my-proj").success());
    const tool_result wrong_config = execute("open my-proj --dry-run --config Release");
    EXPECT_FALSE(wrong_config.success());
    EXPECT_NE(wrong_config.message.find("no Release editor build"), std::string::npos);

    ctx.env = {};
    const tool_result no_env = execute("open my-proj --dry-run");
    EXPECT_FALSE(no_env.success());
    EXPECT_NE(no_env.message.find("no Other Environment found"), std::string::npos);
  }

}  // namespace
