/**
 * \file tests/project/project_compiler_tests.cpp
 *
 * packaging a project: what is checked before anything is written, what ends up in the
 * package, and what a failed compile leaves behind. the runtime, the engine's assemblies and
 * its lua files are stand-ins in a temp folder; the engine's resources are the real ones.
 **/
#include <filesystem>
#include <fstream>
#include <sstream>

#include <asio/asio.hpp>

#include "event/event_system.hpp"
#include "file/filesystem.hpp"
#include "file/path_helpers.hpp"

#include "project/pipeline_collector.hpp"
#include "project/project_compiler.hpp"
#include "vm/builtin_command_devices.hpp"
#include "vm/command_bus.hpp"
#include "vm/command_files/ocmd_toolchain.hpp"

#include "other_test.hpp"

namespace other {
  namespace {

    constexpr std::string_view kProject =
      "# a comment that has to survive the packaging\n"
      "[project]\n"
      "metadata = [ { key = \"name\", value = \"Sandbox Game\" } ]\n"
      "\n"
      "[scripting]\n"
      "projectrc-path = \"game/project.lua\"\n"
      "\n"
      "[scene-graph]\n"
      "starting-scene = \"Main\"\n"
      "scenes = [\n"
      "  { name = \"Main\", path = \"game/assets/scenes/main.oscn\" },\n"
      "  { name = \"Second\", path = \"game/assets/scenes/second.oscn\" },\n"
      "]\n";

    constexpr std::string_view kScene = "[scene]\nschema-version = 1\nname = \"main\"\n";

  }  // namespace

  class project_compiler_tests : public other_test {
   protected:
    void SetUp() override {
      other_test::SetUp();
      const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
      sandbox = std::filesystem::temp_directory_path() / "other-project-compiler-tests" / info->name();
      std::filesystem::remove_all(sandbox);
      project_folder = sandbox / "game";
      output = sandbox / "out";

      /// a compile mounts the project and the engine, and a mount needs somewhere to send its
      ///  change events; a driver sets that up, and so does the command-line runtime
      events = std::make_unique<event_system>(io);
      subsystem<file_system>::get()->initialize_file_events(*events);

      write("game/game.toml", kProject);
      write("game/project.lua", "-- project settings\n");
      write("game/assets/scenes/main.oscn", kScene);
      write("game/assets/scenes/second.oscn", kScene);
      write("game/assets/textures/wall.txt", "a texture, as far as a copy is concerned");
      write("game/assets/vm/startup.ocmd", "on pre_load {\n  core log \"starting\" 2\n}\n");
      /// what a project folder gathers and a package must not have
      write("game/bin/Debug/Game.dll", "an old build");
      write("game/obj/Game.build-errors.log", "");
      write("game/logs/editor.log", "");
      write("game/.git/HEAD", "");
      write("game/Game.csproj", "<Project />");
      write("game/scripts/Thing.cs", "class Thing {}");

      /// stand-ins for the parts of the engine a compile only copies
      write("engine/runtime/other_runtime.exe", "the runtime");
      write("engine/runtime/other_runtime.pdb", "its symbols");
      write("engine/runtime/SDL3.dll", "a library");
      write("engine/runtime/notes.txt", "not a library");
      write("engine/assemblies/OtherCs.dll", "engine scripts");
      write("engine/assemblies/OtherCs.pdb", "their symbols");
      write("engine/assemblies/OtherCsBindings.dll", "bindings");
      write("engine/config/OtherCsBindings.runtimeconfig.json", "{}");
      write("engine/lua/driver.lua", "-- driver");
      write("engine/lua/console/commands.lua", "-- commands");
      write("engine/lua/readme.md", "not lua");
    }

    void TearDown() override {
      other_test::TearDown();
      events = nullptr;
      std::error_code ec;
      std::filesystem::remove_all(sandbox, ec);
    }

    /// path is from the sandbox
    void write(const filepath& path, const std::string_view text) {
      const filepath full = sandbox / path;
      std::filesystem::create_directories(full.parent_path());
      std::ofstream out(full, std::ios::binary | std::ios::trunc);
      ASSERT_TRUE(out.is_open()) << full.string();
      out.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    static std::string read(const filepath& path) {
      std::ifstream in(path, std::ios::binary);
      std::stringstream buffer;
      buffer << in.rdbuf();
      return buffer.str();
    }

    /// the engine's default pipeline under another name, as the collector's tests make one
    void write_pipeline(const filepath& path, const std::string_view name) {
      std::string text = read(resolve_asset_path(kDefaultProjectPipeline).resolved);
      const std::string_view old_name = "name = \"default-instancing\"";
      const size_t at = text.find(old_name);
      ASSERT_NE(at, std::string::npos);
      text.replace(at, old_name.size(), std::format("name = \"{}\"", name));
      write(path, text);
    }

    engine_sources stand_in_engine() const {
      return {
        .runtime_executable = sandbox / "engine/runtime/other_runtime.exe",
        .assemblies = sandbox / "engine/assemblies",
        .dotnet_runtime_config = sandbox / "engine/config/OtherCsBindings.runtimeconfig.json",
        .lua = sandbox / "engine/lua",
        .root = environment_root(),
      };
    }

    compile_settings settings(const package_config config = package_config::RELEASE) const {
      return { .project_file = project_folder, .output_directory = output, .config = config, .engine = stand_in_engine() };
    }

    static compile_report compile(const compile_settings& settings) {
      command_bus bus;
      register_builtin_command_devices(bus);
      return compile_project(bus, settings);
    }

    static bool any_holds(std::span<const std::string> errors, const std::string_view part) {
      return std::ranges::any_of(errors, [part](const std::string& error) { return error.contains(part); });
    }

    bool nothing_was_written() const {
      return !std::filesystem::exists(output);
    }

    asio::io_context io;
    std::unique_ptr<event_system> events;
    filepath sandbox;
    filepath project_folder;
    filepath output;
  };

  TEST_F(project_compiler_tests, a_package_holds_the_runtime_the_project_and_what_they_need_of_the_engine) {
    const compile_report report = compile(settings());
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    const filepath package = output / "Sandbox Game";
    EXPECT_EQ(report.package_root, package);

    /// the runtime under the game's name, with the libraries that sat beside it and nothing else
    EXPECT_EQ(read(package / "Sandbox Game.exe"), "the runtime");
    EXPECT_EQ(read(package / "SDL3.dll"), "a library");
    EXPECT_FALSE(std::filesystem::exists(package / "notes.txt"));
    EXPECT_FALSE(std::filesystem::exists(package / "other_runtime.pdb"));

    /// the config the runtime finds beside itself
    const std::string config = read(package / "Sandbox Game.toml");
    EXPECT_TRUE(config.contains("boot-file = \"game/boot.obin\""));
    EXPECT_TRUE(config.contains("name = \"Sandbox Game\""));
    EXPECT_TRUE(config.contains("core-log-level = \"info\""));
    EXPECT_TRUE(config.contains("core-log-file = \"logs/Sandbox Game.log\""));
    EXPECT_TRUE(config.contains("main-pipeline = \"default-instancing\""));
    EXPECT_TRUE(config.contains("pipelines = [ \"engine://resources/editor-assets/default-pipeline.toml\" ]"));
    EXPECT_TRUE(config.contains("other-lua-directory = \"engine/lua\""));
    EXPECT_FALSE(config.contains("vm-debug-mode"));

    /// of the engine: the pipeline and what it reaches, the fonts, the scripts side
    const filepath engine = package / "engine";
    EXPECT_TRUE(std::filesystem::is_regular_file(engine / "resources/editor-assets/default-pipeline.toml"));
    EXPECT_TRUE(std::filesystem::is_regular_file(engine / "resources/basic-shading.frag"));
    EXPECT_TRUE(std::filesystem::is_regular_file(engine / "resources/shader-modules/camera.glsl"));
    EXPECT_TRUE(std::filesystem::is_regular_file(engine / "resources/smaa-area-tex.bin"));
    EXPECT_FALSE(std::filesystem::is_empty(engine / "resources/fonts"));
    /// and not what no pipeline of this project reaches
    EXPECT_FALSE(std::filesystem::exists(engine / "resources/editor.lua"));
    EXPECT_FALSE(std::filesystem::exists(engine / "resources/models"));
    EXPECT_EQ(read(engine / "lua/driver.lua"), "-- driver");
    EXPECT_EQ(read(engine / "lua/console/commands.lua"), "-- commands");
    EXPECT_FALSE(std::filesystem::exists(engine / "lua/readme.md"));
    EXPECT_EQ(read(engine / "dotnet-assemblies/OtherCs.dll"), "engine scripts");
    EXPECT_EQ(read(engine / "dotnet-assemblies/OtherCsBindings.dll"), "bindings");
    EXPECT_EQ(read(engine / "dotnet-assemblies/OtherCsBindings.runtimeconfig.json"), "{}");
    EXPECT_FALSE(std::filesystem::exists(engine / "dotnet-assemblies/OtherCs.pdb"));

    /// the project: its content, its boot program, and none of its sources or build leftovers
    const filepath game = package / "game";
    EXPECT_EQ(read(game / "assets/scenes/main.oscn"), kScene);
    EXPECT_EQ(read(game / "assets/textures/wall.txt"), "a texture, as far as a copy is concerned");
    EXPECT_EQ(read(game / "project.lua"), "-- project settings\n");
    EXPECT_FALSE(ocmd_toolchain{}.read_obin(game / "boot.obin").empty());
    for (const std::string_view left_out : { "assets/vm", "bin", "obj", "logs", ".git", "Game.csproj", "scripts/Thing.cs" }) {
      EXPECT_FALSE(std::filesystem::exists(game / left_out)) << left_out;
    }

    /// the project file as the package needs it, comments and all
    const std::string packaged = read(game / "game.toml");
    EXPECT_TRUE(packaged.starts_with("# a comment that has to survive the packaging\n"));
    EXPECT_TRUE(packaged.contains("[vm]\nboot-file = \"boot.obin\"\n"));
    EXPECT_TRUE(packaged.contains("starting-scene = \"Main\"\n"));
    EXPECT_EQ(read(project_folder / "game.toml"), kProject);

    EXPECT_TRUE(std::filesystem::exists(package / kPackageMarker));
    EXPECT_FALSE(std::filesystem::exists(output / ".staging"));
    EXPECT_TRUE(report.warnings.empty());
  }

  TEST_F(project_compiler_tests, the_projects_plugins_are_packaged_in_the_packages_own_configuration) {
    /// built outside the project's folder, one folder to a configuration, as CMake leaves them
    write("build/game/Release/systems.dll", "release systems");
    write("build/game/Release/systems.pdb", "release symbols");
    write("build/game/Debug/systems.dll", "debug systems");
    write("build/game/Debug/systems.pdb", "debug symbols");
    write("game/tools/fixed.dll", "one build only");
    const std::string with_plugins = std::format(
      "[project]\nplugins = [\n"
      "  {{ name = \"Game Systems\", path = \"build/game/${{build-config}}/systems.dll\" }},  # the game's own\n"
      "  {{ name = \"Fixed\", path = 'game/tools/fixed.dll' }}\n"
      "]\n{}",
      std::string_view{ kProject }.substr(std::string_view{ kProject }.find("metadata")));
    write("game/game.toml", with_plugins);

    compile_report report = compile(settings());
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    filepath game = report.package_root / "game";
    EXPECT_EQ(read(game / "bin/systems.dll"), "release systems");
    EXPECT_EQ(read(game / "bin/fixed.dll"), "one build only");
    EXPECT_FALSE(std::filesystem::exists(game / "bin/systems.pdb"));
    /// the packaged file points at the copies; the rest of each line is as it was
    std::string packaged = read(game / "game.toml");
    EXPECT_TRUE(packaged.contains("  { name = \"Game Systems\", path = \"game/bin/systems.dll\" },  # the game's own\n"));
    EXPECT_TRUE(packaged.contains("  { name = \"Fixed\", path = \"game/bin/fixed.dll\" }\n"));
    EXPECT_FALSE(packaged.contains("build-config"));
    EXPECT_EQ(read(project_folder / "game.toml"), with_plugins);

    /// a Debug package takes the Debug build, whatever this test was built as, and its symbols
    report = compile(settings(package_config::DEBUG));
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    game = report.package_root / "game";
    EXPECT_EQ(read(game / "bin/systems.dll"), "debug systems");
    EXPECT_EQ(read(game / "bin/systems.pdb"), "debug symbols");
  }

  TEST_F(project_compiler_tests, a_plugin_that_cannot_be_packaged_stops_the_compile_before_it_starts) {
    const std::string_view rest = std::string_view{ kProject }.substr(std::string_view{ kProject }.find("metadata"));
    write("build/game/Debug/systems.dll", "debug only");

    /// not built in the configuration being packaged
    write("game/game.toml", std::format("[project]\nplugins = [ {{ name = \"Game Systems\", path = \"build/game/${{build-config}}/systems.dll\" }} ]\n{}", rest));
    compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "plugin 'Game Systems' is listed as 'build/game/${build-config}/systems.dll', and there is no Release build of it at 'build/game/Release/systems.dll'"));
    EXPECT_TRUE(nothing_was_written());

    /// two that would land on each other in the package
    write("game/other/systems.dll", "another");
    write("game/game.toml", std::format("[project]\nplugins = [ {{ name = \"One\", path = \"build/game/Debug/systems.dll\" }}, {{ name = \"Two\", path = \"game/other/systems.dll\" }} ]\n{}", rest));
    report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "two plugins are both named 'systems.dll'"));

    write("game/game.toml", std::format("[project]\nplugins = [ {{ name = \"No Path\" }} ]\n{}", rest));
    report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "has no name or no path"));
    EXPECT_TRUE(nothing_was_written());
  }

  TEST_F(project_compiler_tests, a_debug_package_has_its_own_folder_its_symbols_and_a_louder_log) {
    const compile_report report = compile(settings(package_config::DEBUG));
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    const filepath package = output / "Sandbox Game-debug";
    EXPECT_EQ(report.package_root, package);

    EXPECT_EQ(read(package / "Sandbox Game.exe"), "the runtime");
    EXPECT_EQ(read(package / "other_runtime.pdb"), "its symbols");
    EXPECT_EQ(read(package / "engine/dotnet-assemblies/OtherCs.pdb"), "their symbols");
    const std::string config = read(package / "Sandbox Game.toml");
    EXPECT_TRUE(config.contains("core-log-level = \"debug\""));
    EXPECT_TRUE(config.contains("vm-debug-mode = false"));
    /// said every time, since it is the first thing that goes wrong on another machine
    EXPECT_TRUE(any_holds(report.warnings, "needs the debug C runtime"));

    /// and a Release compile beside it does not disturb it
    ASSERT_TRUE(compile(settings()).success);
    EXPECT_TRUE(std::filesystem::exists(package / "Sandbox Game.exe"));
    EXPECT_TRUE(std::filesystem::exists(output / "Sandbox Game" / "Sandbox Game.exe"));
  }

  TEST_F(project_compiler_tests, the_public_name_is_the_callers_then_the_projects_then_its_metadata_then_its_folder) {
    compile_settings named = settings();
    named.public_name = "Chosen Here";
    ASSERT_TRUE(compile(named).success);
    EXPECT_TRUE(std::filesystem::exists(output / "Chosen Here" / "Chosen Here.exe"));
    /// the folder inside the package keeps the project's own name, whatever the game is called
    EXPECT_TRUE(std::filesystem::exists(output / "Chosen Here" / "game" / "game.toml"));

    write("game/game.toml", set_toml_string(kProject, "project", "public-name", "From The File"));
    ASSERT_TRUE(compile(settings()).success);
    EXPECT_TRUE(std::filesystem::exists(output / "From The File" / "From The File.exe"));

    write("game/game.toml", "[scene-graph]\nstarting-scene = \"Main\"\nscenes = [ { name = \"Main\", path = \"game/assets/scenes/main.oscn\" } ]\n");
    ASSERT_TRUE(compile(settings()).success);
    EXPECT_TRUE(std::filesystem::exists(output / "game" / "game.exe"));
  }

  TEST_F(project_compiler_tests, a_public_name_has_to_be_a_file_name) {
    for (const std::string_view bad : { "", "a/b", "a\\b", "what?", "star*", "quo\"te", "pipe|d", "<tag>", "trailing.", "trailing ", " leading", "CON", "nul", "Com1", "LPT9.txt" }) {
      EXPECT_TRUE(invalid_public_name_reason(bad).has_value()) << "'" << bad << "'";
    }
    for (const std::string_view good : { "My Game", "game-2", "Console", "COM10", "a.b", "über" }) {
      EXPECT_FALSE(invalid_public_name_reason(good).has_value()) << "'" << good << "'";
    }

    compile_settings named = settings();
    named.public_name = "What?";
    const compile_report report = compile(named);
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the public name 'What?' cannot be used"));
    EXPECT_TRUE(nothing_was_written());
  }

  TEST_F(project_compiler_tests, a_package_needs_a_starting_scene_the_project_has) {
    const std::string no_start = remove_toml_key(kProject, "scene-graph", "starting-scene");
    write("game/game.toml", no_start);
    compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the project has no starting scene; choose one of: Main, Second"));
    EXPECT_TRUE(nothing_was_written());

    /// the caller's choice goes into the package and not back into the project
    compile_settings chosen = settings();
    chosen.starting_scene = "Second";
    report = compile(chosen);
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    EXPECT_TRUE(read(report.package_root / "game" / "game.toml").contains("[scene-graph]\nstarting-scene = \"Second\"\n"));
    EXPECT_EQ(read(project_folder / "game.toml"), no_start);

    chosen.starting_scene = "Nowhere";
    report = compile(chosen);
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the starting scene 'Nowhere' is not one of the project's scenes: Main, Second"));
  }

  TEST_F(project_compiler_tests, a_project_with_no_scenes_or_a_scene_that_is_missing_is_not_packaged) {
    write("game/game.toml", "[project]\n");
    compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the project has no scenes"));

    write("game/game.toml", kProject);
    std::filesystem::remove(project_folder / "assets/scenes/second.oscn");
    report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "scene 'Second' is listed as 'game/assets/scenes/second.oscn', which does not exist"));

    /// a scene kept somewhere else would not be in the package
    write("elsewhere/outside.oscn", kScene);
    write("game/game.toml", "[scene-graph]\nstarting-scene = \"Out\"\nscenes = [ { name = \"Out\", path = \"elsewhere/outside.oscn\" } ]\n");
    report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "outside the project's folder"));
    EXPECT_TRUE(nothing_was_written());
  }

  TEST_F(project_compiler_tests, a_failed_compile_leaves_the_last_good_package_as_it_was) {
    ASSERT_TRUE(compile(settings()).success);
    const filepath package = output / "Sandbox Game";
    const std::string good_program = read(package / "game" / "boot.obin");

    write("game/assets/vm/startup.ocmd", "on pre_load {\n  this is not a command\n}\n");
    const compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the project's VM sources did not build"));
    EXPECT_TRUE(report.package_root.empty());

    EXPECT_EQ(read(package / "game" / "boot.obin"), good_program);
    EXPECT_TRUE(std::filesystem::exists(package / "Sandbox Game.exe"));
    EXPECT_FALSE(std::filesystem::exists(output / ".staging"));
  }

  TEST_F(project_compiler_tests, a_compile_replaces_its_own_package_and_no_other_folder) {
    /// someone's own folder happens to have the game's name
    write("out/Sandbox Game/save-games/slot-1.dat", "hours of play");
    compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "was not made by a compile"));
    EXPECT_EQ(read(output / "Sandbox Game/save-games/slot-1.dat"), "hours of play");
    EXPECT_FALSE(std::filesystem::exists(output / ".staging"));

    /// its own it replaces whole: what the last package had and this one does not is gone
    std::filesystem::remove_all(output);
    ASSERT_TRUE(compile(settings()).success);
    write("out/Sandbox Game/game/assets/textures/stale.txt", "from an older package");
    ASSERT_TRUE(compile(settings()).success);
    EXPECT_FALSE(std::filesystem::exists(output / "Sandbox Game/game/assets/textures/stale.txt"));
    EXPECT_TRUE(std::filesystem::exists(output / "Sandbox Game/game/assets/textures/wall.txt"));
  }

  TEST_F(project_compiler_tests, the_package_cannot_be_made_inside_the_project) {
    compile_settings inside = settings();
    inside.output_directory = project_folder / "packages";
    const compile_report report = compile(inside);
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "is inside the project"));
    EXPECT_FALSE(std::filesystem::exists(project_folder / "packages"));
  }

  TEST_F(project_compiler_tests, an_assembly_the_project_already_has_ships_as_it_is) {
    write("game/prebuilt/Game.dll", "built somewhere else");
    write("game/game.toml", set_toml_string(kProject, "scripting", "assembly", "game/prebuilt/Game.dll"));

    const compile_report report = compile(settings());
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    const filepath game = report.package_root / "game";
    EXPECT_EQ(read(game / "bin/Game.dll"), "built somewhere else");
    EXPECT_TRUE(read(game / "game.toml").contains("assembly = \"game/bin/Game.dll\"\n"));

    write("game/game.toml", set_toml_string(kProject, "scripting", "assembly", "game/prebuilt/Gone.dll"));
    const compile_report missing = compile(settings());
    EXPECT_FALSE(missing.success);
    EXPECT_TRUE(any_holds(missing.errors, "the project's script assembly, 'game/prebuilt/Gone.dll', does not exist"));
  }

  TEST_F(project_compiler_tests, a_projects_own_pipeline_is_listed_from_its_folder_and_the_main_one_has_to_be_known) {
    write_pipeline("game/assets/pipelines/mine.toml", "mine");
    const std::string two = std::format("{}\n[rendering]\npipelines = [ \"assets/pipelines/mine.toml\", \"{}\" ]\n", kProject, kDefaultProjectPipeline);
    write("game/game.toml", two);

    compile_report report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "does not say which one reaches the window; choose one of: mine, default-instancing"));

    compile_settings chosen = settings();
    chosen.main_pipeline = "mine";
    report = compile(chosen);
    ASSERT_TRUE(report.success) << (report.errors.empty() ? "" : report.errors.front());
    const std::string config = read(report.package_root / "Sandbox Game.toml");
    EXPECT_TRUE(config.contains("main-pipeline = \"mine\""));
    EXPECT_TRUE(config.contains("pipelines = [ \"game/assets/pipelines/mine.toml\", \"engine://resources/editor-assets/default-pipeline.toml\" ]"));
    /// the project's pipeline travels with the project, and the engine shaders it uses with the engine
    EXPECT_TRUE(std::filesystem::is_regular_file(report.package_root / "game/assets/pipelines/mine.toml"));
    EXPECT_TRUE(std::filesystem::is_regular_file(report.package_root / "engine/resources/basic-shading.frag"));

    write("game/game.toml", std::format("{}main-pipeline = \"missing\"\n", two));
    report = compile(settings());
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the main pipeline 'missing' is not one of the project's pipelines"));
  }

  TEST_F(project_compiler_tests, a_runtime_that_has_not_been_built_stops_the_compile_before_it_starts) {
    compile_settings no_runtime = settings();
    no_runtime.engine->runtime_executable = sandbox / "engine/runtime/gone.exe";
    const compile_report report = compile(no_runtime);
    EXPECT_FALSE(report.success);
    EXPECT_TRUE(any_holds(report.errors, "the runtime to package"));
    EXPECT_TRUE(nothing_was_written());

    compile_settings no_project = settings();
    no_project.project_file = sandbox / "nothing-here";
    EXPECT_FALSE(compile(no_project).success);
    EXPECT_TRUE(nothing_was_written());
  }

  TEST_F(project_compiler_tests, a_value_in_a_project_file_is_set_without_disturbing_the_rest) {
    const std::string_view text = "# top\n[vm]\n  boot-file = \"assets/vm/boot.oasm\"  \n# between\n[scripting]\ncs_project = \"game/Game.csproj\"\n";

    /// replaced where it is, with its indent
    EXPECT_EQ(set_toml_string(text, "vm", "boot-file", "boot.obin"),
              "# top\n[vm]\n  boot-file = \"boot.obin\"\n# between\n[scripting]\ncs_project = \"game/Game.csproj\"\n");
    /// added under the table's header when the table has no such key
    EXPECT_EQ(set_toml_string(text, "scripting", "assembly", "game/bin/Game.dll"),
              "# top\n[vm]\n  boot-file = \"assets/vm/boot.oasm\"  \n# between\n[scripting]\nassembly = \"game/bin/Game.dll\"\ncs_project = \"game/Game.csproj\"\n");
    /// a key of the same name in another table is not this table's
    EXPECT_EQ(set_toml_string(text, "rendering", "boot-file", "x"), std::format("{}\n[rendering]\nboot-file = \"x\"\n", text));
    /// and a table that is not there is added at the end, to a file with or without a last line break
    EXPECT_EQ(set_toml_string("[a]\nb = 1", "c", "d", "e"), "[a]\nb = 1\n\n[c]\nd = \"e\"\n");
    EXPECT_EQ(set_toml_string("", "c", "d", "e"), "[c]\nd = \"e\"\n");

    EXPECT_EQ(remove_toml_key(text, "scripting", "cs_project"), "# top\n[vm]\n  boot-file = \"assets/vm/boot.oasm\"  \n# between\n[scripting]\n");
    EXPECT_EQ(remove_toml_key(text, "vm", "cs_project"), text);
    EXPECT_EQ(remove_toml_key(text, "nowhere", "cs_project"), text);

    /// the file's own line endings are the ones used, and a value is written as a TOML string
    EXPECT_EQ(set_toml_string("[a]\r\nb = \"old\"\r\n", "a", "c", "say \"hi\""), "[a]\r\nc = \"say \\\"hi\\\"\"\r\nb = \"old\"\r\n");
    EXPECT_EQ(set_toml_string("[a]\r\nb = \"old\"\r\n", "a", "b", "new"), "[a]\r\nb = \"new\"\r\n");
    /// a header with a comment after it is still the header
    EXPECT_EQ(set_toml_string("[a] # the a table\nb = 1\n", "a", "b", "2"), "[a] # the a table\nb = \"2\"\n");
  }

}  // namespace other
