/**
 * \file tests/cli/scene_tool_tests.cpp
 *
 * the oecli scene tool: compile/decompile/info over hermetic temp files, in-process
 * through the same registry the executable and in-environment hosts use.
 **/
#include <gtest/gtest.h>

#include <fstream>

#include "other_test.hpp"

#include "cli/tool_registry.hpp"
#include "serialization/scene_serializer.hpp"
#include "tools/scene_cli_tool.hpp"

namespace other {

  class scene_tool_tests : public other_test {
   protected:
    void SetUp() override {
      other_test::SetUp();
      test_root = std::filesystem::temp_directory_path() / "other-scene-tool-tests" / ::testing::UnitTest::GetInstance()->current_test_info()->name();
      std::filesystem::remove_all(test_root);
      std::filesystem::create_directories(test_root);

      ctx = {};
      ctx.working_directory = test_root;
      ctx.out = [this](std::string_view message) { output += std::format("{}\n", message); };
      ctx.err = [this](std::string_view message) { errors += std::format("{}\n", message); };

      cli::register_environment_tools(registry);
    }

    void TearDown() override {
      std::error_code ec;
      std::filesystem::remove_all(test_root, ec);
      other_test::TearDown();
    }

    void write_file(const filepath& path, std::string_view contents) {
      std::ofstream out(path, std::ios::binary | std::ios::trunc);
      ASSERT_TRUE(out.is_open()) << path.string();
      out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    }

    cli::tool_result run_scene_tool(std::string_view line) {
      return registry.execute_line(ctx, line);
    }

    constexpr static std::string_view kSceneToml = R"(# test scene
[scene]
schema-version = 1
name = "tool-test"
clear-color = [0.1, 0.2, 0.3, 1.0]

[[objects]]
id = 1
name = "Thing"
tags = ["main-camera"]

[objects.components.transform]
local_position = [1.0, 2.0, 3.0]

[[objects]]
id = 2
parent = 1
name = "ChildThing"
)";

    cli::tool_registry registry;
    cli::tool_context ctx;
    filepath test_root;
    std::string output;
    std::string errors;
  };

  TEST_F(scene_tool_tests, compile_produces_a_loadable_binary) {
    write_file(test_root / "scene.oscn", kSceneToml);

    const cli::tool_result result = run_scene_tool("scene compile scene.oscn");
    EXPECT_TRUE(result.success()) << result.message;

    const filepath compiled = test_root / "scene.oscnb";
    ASSERT_TRUE(std::filesystem::exists(compiled));

    serialization::scene_parse_result parsed = serialization::load_scene_document(compiled);
    ASSERT_TRUE(parsed.success()) << parsed.error;
    EXPECT_EQ(parsed.document->name, "tool-test");
    ASSERT_EQ(parsed.document->objects.size(), 2u);
    EXPECT_EQ(parsed.document->objects[0].name, "Thing");
    EXPECT_EQ(parsed.document->objects[1].parent_file_id, 1u);
  }

  TEST_F(scene_tool_tests, compile_then_decompile_roundtrips_the_document) {
    write_file(test_root / "scene.oscn", kSceneToml);

    ASSERT_TRUE(run_scene_tool("scene compile scene.oscn").success());
    ASSERT_TRUE(run_scene_tool("scene decompile scene.oscnb -o roundtrip.oscn").success());

    serialization::scene_parse_result original = serialization::load_scene_document(test_root / "scene.oscn");
    serialization::scene_parse_result roundtrip = serialization::load_scene_document(test_root / "roundtrip.oscn");
    ASSERT_TRUE(original.success());
    ASSERT_TRUE(roundtrip.success()) << roundtrip.error;

    ASSERT_EQ(original.document->objects.size(), roundtrip.document->objects.size());
    for (size_t i = 0; i < original.document->objects.size(); ++i) {
      EXPECT_EQ(original.document->objects[i].name, roundtrip.document->objects[i].name);
      EXPECT_EQ(original.document->objects[i].parent_file_id, roundtrip.document->objects[i].parent_file_id);
      EXPECT_EQ(original.document->objects[i].tags, roundtrip.document->objects[i].tags);
      ASSERT_EQ(original.document->objects[i].components.size(), roundtrip.document->objects[i].components.size());
      for (size_t c = 0; c < original.document->objects[i].components.size(); ++c) {
        EXPECT_EQ(original.document->objects[i].components[c].payload, roundtrip.document->objects[i].components[c].payload);
      }
    }
  }

  TEST_F(scene_tool_tests, info_summarizes_the_document) {
    write_file(test_root / "scene.oscn", kSceneToml);

    const cli::tool_result result = run_scene_tool("scene info scene.oscn");
    EXPECT_TRUE(result.success()) << result.message;
    EXPECT_NE(output.find("tool-test"), std::string::npos);
    EXPECT_NE(output.find("Thing"), std::string::npos);
    EXPECT_NE(output.find("main-camera"), std::string::npos);
  }

  TEST_F(scene_tool_tests, user_errors_report_without_terminating) {
    EXPECT_FALSE(run_scene_tool("scene").success());
    EXPECT_FALSE(run_scene_tool("scene frobnicate x.oscn").success());
    EXPECT_FALSE(run_scene_tool("scene compile missing.oscn").success());

    write_file(test_root / "broken.oscn", "not [ valid toml");
    EXPECT_FALSE(run_scene_tool("scene compile broken.oscn").success());

    write_file(test_root / "wrong.oscnb", "OOPS");
    EXPECT_FALSE(run_scene_tool("scene decompile wrong.oscnb").success());

    /// wrong extension for the command
    write_file(test_root / "scene.oscn", kSceneToml);
    EXPECT_FALSE(run_scene_tool("scene decompile scene.oscn").success());
  }

}  // namespace other
