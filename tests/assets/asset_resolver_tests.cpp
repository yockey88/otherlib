/**
 * \file tests/assets/asset_resolver_tests.cpp
 *
 * contract under test: asset_resolver builds a leaves-first dependency snapshot
 *  from an SDK-style csproj (implicit compile glob, default excludes, produces-edge
 *  assembly artifact) and re_resolve yields correct deltas for the hot-reload flows:
 *  modified .cs, added .cs, deleted .cs, manifest self-edit, and excluded-path noise.
 **/
#include <fstream>

#include <asio/asio.hpp>
#include <gtest/gtest.h>

#include "event/event_system.hpp"
#include "file/filesystem.hpp"
#include "file/path_helpers.hpp"

#include "asset/asset_resolver.hpp"
#include "other_test.hpp"

namespace other {

  /// hermetic fake project mirroring SpaceSim.csproj's shape (SDK-style, no <Compile>
  /// items, conditional <Reference HintPath> on $(Configuration)):
  ///   <temp>/fake-proj/FakeProj.csproj
  ///   <temp>/fake-proj/scripts/a.cs
  ///   <temp>/fake-proj/scripts/b.cs
  ///   <temp>/fake-proj/bin/Debug/junk.cs      (excluded by the implicit glob)
  class asset_resolver_tests : public other_test {
   protected:
    static constexpr std::string_view kMountName = "fakeproj";

    asio::io_context io;
    filepath proj_root;

    void SetUp() override {
      other_test::SetUp();

      proj_root = std::filesystem::temp_directory_path() / "other-resolver-tests" / "fake-proj";
      std::filesystem::remove_all(proj_root.parent_path());
      std::filesystem::create_directories(proj_root / "scripts");
      std::filesystem::create_directories(proj_root / "bin" / "Debug");

      write_file(csproj_path(), default_csproj());
      write_file(proj_root / "scripts" / "a.cs", "class A {}");
      write_file(proj_root / "scripts" / "b.cs", "class B {}");
      write_file(proj_root / "bin" / "Debug" / "junk.cs", "class Junk {}");

      events = std::make_unique<event_system>(io);
      auto* fs = subsystem<file_system>::get();
      fs->initialize_file_events(*events);
      ref<directory> mount = fs->mount_directory(kMountName, proj_root, mount_scope::PROJECT);
      OTHER_ASSERT(mount != nullptr, "failed to mount fake project root");
    }

    void TearDown() override {
      other_test::TearDown();
      events = nullptr;
      std::error_code ec;
      std::filesystem::remove_all(proj_root.parent_path(), ec);
    }

    filepath csproj_path() const { return proj_root / "FakeProj.csproj"; }

    static std::string default_csproj(const std::string_view extra_items = "") {
      return std::format(
        R"(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Library</OutputType>
    <TargetFramework>net9.0</TargetFramework>
  </PropertyGroup>
  <ItemGroup Condition="'$(Configuration)' == 'Debug'">
    <Reference Include="SomeLocalLib">
      <HintPath>C:\definitely\not\portable\SomeLocalLib.dll</HintPath>
    </Reference>
  </ItemGroup>
{}</Project>)",
        extra_items);
    }

    static void write_file(const filepath& path, std::string_view contents) {
      std::ofstream out(path);
      out << contents;
    }

    natural_t stable_of(const filepath& abs) const { return stable_id_for(virtualize(abs)); }

    /// the produces-edge artifact interned by resolve(); path embeds the dotnet config
    natural_t produced_dll_stable() const {
      return stable_id_for(std::format("{}/bin/{}/FakeProj.dll", kMountName, get_project_build_config_string()));
    }

    std::unique_ptr<event_system> events;
  };

  TEST_F(asset_resolver_tests, resolve_builds_layers_leaves_first) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// csproj + a.cs + b.cs + produces-edge dll artifact; bin/**/junk.cs is excluded
    ASSERT_EQ(snap.nodes.size(), 4u);
    EXPECT_NE(snap.find(stable_of(csproj_path())), nullptr);
    EXPECT_NE(snap.find(stable_of(proj_root / "scripts" / "a.cs")), nullptr);
    EXPECT_NE(snap.find(stable_of(proj_root / "scripts" / "b.cs")), nullptr);
    EXPECT_EQ(snap.find(stable_of(proj_root / "bin" / "Debug" / "junk.cs")), nullptr);

    const dependency_snapshot::node* dll = snap.find(produced_dll_stable());
    ASSERT_NE(dll, nullptr);
    EXPECT_EQ(dll->type, asset::SCRIPT_SOURCE);

    /// layers: [a.cs, b.cs] -> [csproj] -> [dll]
    ASSERT_EQ(snap.topo_layers.size(), 3u);
    EXPECT_EQ(snap.topo_layers[0].size(), 2u);
    EXPECT_EQ(snap.topo_layers[1].size(), 1u);
    EXPECT_EQ(snap.topo_layers[2].size(), 1u);
    EXPECT_EQ(snap.nodes[snap.topo_layers[1][0]].stable_id, stable_of(csproj_path()));
  }

  TEST_F(asset_resolver_tests, stable_ids_are_mount_relative) {
    EXPECT_EQ(virtualize(std::filesystem::absolute(csproj_path())), std::string{ kMountName } + "/FakeProj.csproj");
    EXPECT_EQ(virtualize(std::filesystem::absolute(proj_root / "scripts" / "a.cs")), std::string{ kMountName } + "/scripts/a.cs");
  }

  TEST_F(asset_resolver_tests, re_resolve_modified_cs_yields_modified_plus_affected_parents) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    write_file(proj_root / "scripts" / "a.cs", "class A { /* edited */ }");
    const resolve_delta delta = resolver.re_resolve(snap, proj_root / "scripts" / "a.cs");

    ASSERT_FALSE(delta.empty());
    EXPECT_TRUE(std::ranges::contains(delta.modified, stable_of(proj_root / "scripts" / "a.cs")));
    /// affected closure walks reverse edges: csproj (rebuild) then the dll (reattach)
    EXPECT_TRUE(std::ranges::contains(delta.affected_parents, stable_of(csproj_path())));
    EXPECT_TRUE(std::ranges::contains(delta.affected_parents, produced_dll_stable()));
    EXPECT_TRUE(delta.added.empty());
    EXPECT_TRUE(delta.removed.empty());
  }

  TEST_F(asset_resolver_tests, re_resolve_added_cs_yields_added_plus_affected_parent) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    write_file(proj_root / "scripts" / "c.cs", "class C {}");
    const resolve_delta delta = resolver.re_resolve(snap, proj_root / "scripts" / "c.cs");

    ASSERT_FALSE(delta.empty());
    EXPECT_TRUE(std::ranges::contains(delta.added, stable_of(proj_root / "scripts" / "c.cs")));
    EXPECT_TRUE(std::ranges::contains(delta.affected_parents, stable_of(csproj_path())));
    EXPECT_NE(snap.find(stable_of(proj_root / "scripts" / "c.cs")), nullptr);
  }

  TEST_F(asset_resolver_tests, re_resolve_deleted_cs_gc_removes_orphan) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);
    const natural_t b_stable = stable_of(proj_root / "scripts" / "b.cs");

    const filepath b_path = proj_root / "scripts" / "b.cs";
    std::filesystem::remove(b_path);
    const resolve_delta delta = resolver.re_resolve(snap, b_path);

    ASSERT_FALSE(delta.empty());
    EXPECT_TRUE(std::ranges::contains(delta.removed, b_stable));
    EXPECT_TRUE(std::ranges::contains(delta.affected_parents, stable_of(csproj_path())));
    EXPECT_EQ(snap.find(b_stable), nullptr);
  }

  /// §3.1 regression: an edited manifest must re-parse ITSELF, not only react to
  /// domain hits and parent walks
  TEST_F(asset_resolver_tests, re_resolve_manifest_edit_reparses_itself) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);
    const natural_t b_stable = stable_of(proj_root / "scripts" / "b.cs");
    ASSERT_NE(snap.find(b_stable), nullptr);

    write_file(csproj_path(), default_csproj("  <ItemGroup>\n    <Compile Remove=\"scripts/b.cs\" />\n  </ItemGroup>\n"));
    const resolve_delta delta = resolver.re_resolve(snap, csproj_path());

    ASSERT_FALSE(delta.empty());
    EXPECT_TRUE(std::ranges::contains(delta.modified, stable_of(csproj_path())));
    EXPECT_TRUE(std::ranges::contains(delta.removed, b_stable));
    EXPECT_EQ(snap.find(b_stable), nullptr);
  }

  /// bug-exclude-tripwire regression: excluded paths must classify-and-continue.
  /// unknown excluded noise (obj churn) is an empty delta; the known produces-edge
  /// artifact (the built dll) is a modified node despite matching bin/**
  TEST_F(asset_resolver_tests, excluded_paths_do_not_assert) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    const resolve_delta noise = resolver.re_resolve(snap, proj_root / "bin" / "Debug" / "junk.cs");
    EXPECT_TRUE(noise.empty());

    const filepath dll_path = proj_root / "bin" / get_project_build_config_string() / "FakeProj.dll";
    const resolve_delta dll_delta = resolver.re_resolve(snap, dll_path);
    ASSERT_FALSE(dll_delta.empty());
    EXPECT_TRUE(std::ranges::contains(dll_delta.modified, produced_dll_stable()));
    EXPECT_TRUE(dll_delta.affected_parents.empty());
  }

  TEST_F(asset_resolver_tests, effective_hash_ignores_untouched_manifest) {
    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// a re_resolve with no on-disk change reports nothing structural: the .cs is
    /// still a known node (modified), but no edges move and nothing is added/removed
    const resolve_delta delta = resolver.re_resolve(snap, proj_root / "scripts" / "a.cs");
    EXPECT_TRUE(delta.added.empty());
    EXPECT_TRUE(delta.removed.empty());
    EXPECT_EQ(snap.nodes.size(), 4u);
  }

  /// csproj parse anchors (through resolve, the only public surface)

  TEST_F(asset_resolver_tests, csproj_conditional_group_selected_by_config) {
    /// tests run as a Debug/ProfileD engine build => dotnet config "Debug":
    /// a Release-conditioned Remove must not apply, a Debug-conditioned one must
    write_file(csproj_path(), default_csproj("  <ItemGroup Condition=\"'$(Configuration)' == 'Release'\">\n"
                                             "    <Compile Remove=\"scripts/a.cs\" />\n"
                                             "  </ItemGroup>\n"
                                             "  <ItemGroup Condition=\"'$(Configuration)' == 'Debug'\">\n"
                                             "    <Compile Remove=\"scripts/b.cs\" />\n"
                                             "  </ItemGroup>\n"));

    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    const bool debug_config = get_project_build_config_string() == "Debug";
    const filepath kept = proj_root / "scripts" / (debug_config ? "a.cs" : "b.cs");
    const filepath removed = proj_root / "scripts" / (debug_config ? "b.cs" : "a.cs");

    EXPECT_NE(snap.find(stable_of(kept)), nullptr);
    EXPECT_EQ(snap.find(stable_of(removed)), nullptr);
  }

  TEST_F(asset_resolver_tests, csproj_semicolon_split_and_backslash_normalization) {
    write_file(csproj_path(), default_csproj("  <ItemGroup>\n"
                                             "    <Compile Remove=\"scripts\\a.cs;scripts\\b.cs\" />\n"
                                             "  </ItemGroup>\n"));

    asset_resolver resolver;
    const std::array roots{ csproj_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// both removes applied despite windows separators and the ';' list form
    EXPECT_EQ(snap.find(stable_of(proj_root / "scripts" / "a.cs")), nullptr);
    EXPECT_EQ(snap.find(stable_of(proj_root / "scripts" / "b.cs")), nullptr);
    EXPECT_EQ(snap.nodes.size(), 2u);  /// csproj + produced dll only
  }

  /// scene manifest anchors (parsers[SCENE]): a scene document declares its hook
  /// script (relative to the scene dir) and every component asset reference

  class scene_manifest_tests : public asset_resolver_tests {
   protected:
    void SetUp() override {
      asset_resolver_tests::SetUp();
      std::filesystem::create_directories(proj_root / "scenes");
      std::filesystem::create_directories(proj_root / "models");
      write_file(proj_root / "scenes" / "hooks.lua", "function OnSceneLoad() end");
      write_file(proj_root / "models" / "ship.fbx", "not a real model");
      write_file(scene_path(), scene_toml({ model_path("ship.fbx") }));
    }

    filepath scene_path() const { return proj_root / "scenes" / "test.oscn"; }
    filepath model_path(const std::string_view name) const { return proj_root / "models" / name; }

    /// component refs are load paths; absolute + forward slashes keeps the test
    /// independent of the runner's working directory
    std::string scene_toml(const ostd::vector<filepath>& models) const {
      std::string text =
        "[scene]\n"
        "schema-version = 1\n"
        "name = \"resolver-scene\"\n"
        "script = \"hooks.lua\"\n";
      for (size_t i = 0; i < models.size(); ++i) {
        text += std::format(
          "\n[[objects]]\n"
          "id = {}\n"
          "name = \"Object{}\"\n"
          "\n[objects.components.render]\n"
          "model_asset_id = \"{}\"\n",
          i + 1, i + 1, models[i].generic_string());
      }
      return text;
    }
  };

  TEST_F(scene_manifest_tests, scene_declares_hook_script_and_model_edges) {
    asset_resolver resolver;
    const std::array roots{ scene_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    ASSERT_EQ(snap.nodes.size(), 3u);
    const dependency_snapshot::node* scene_node = snap.find(stable_of(scene_path()));
    const dependency_snapshot::node* script_node = snap.find(stable_of(proj_root / "scenes" / "hooks.lua"));
    const dependency_snapshot::node* model_node = snap.find(stable_of(model_path("ship.fbx")));
    ASSERT_NE(scene_node, nullptr);
    ASSERT_NE(script_node, nullptr);
    ASSERT_NE(model_node, nullptr);
    EXPECT_EQ(scene_node->type, asset::SCENE);
    EXPECT_EQ(script_node->type, asset::SCRIPT_FILE);
    EXPECT_EQ(model_node->type, asset::MODEL_SOURCE);

    /// leaves first: [hooks.lua, ship.fbx] -> [scene]
    ASSERT_EQ(snap.topo_layers.size(), 2u);
    EXPECT_EQ(snap.topo_layers[0].size(), 2u);
    ASSERT_EQ(snap.topo_layers[1].size(), 1u);
    EXPECT_EQ(snap.nodes[snap.topo_layers[1][0]].stable_id, stable_of(scene_path()));
  }

  TEST_F(scene_manifest_tests, missing_scene_refs_skip_edges_without_asserting) {
    write_file(scene_path(), scene_toml({ model_path("ship.fbx"), model_path("missing.fbx") }));

    asset_resolver resolver;
    const std::array roots{ scene_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// the dangling ref is a data error: warn + skip, never a node or an assert
    EXPECT_EQ(snap.nodes.size(), 3u);
    EXPECT_EQ(snap.find(stable_id_for(std::string{ kMountName } + "/models/missing.fbx")), nullptr);
    EXPECT_NE(snap.find(stable_of(model_path("ship.fbx"))), nullptr);
  }

  TEST_F(scene_manifest_tests, re_resolve_scene_edit_adds_model_edge) {
    asset_resolver resolver;
    const std::array roots{ scene_path() };
    dependency_snapshot snap = resolver.resolve(roots);
    ASSERT_EQ(snap.nodes.size(), 3u);

    write_file(model_path("station.fbx"), "also not a real model");
    write_file(scene_path(), scene_toml({ model_path("ship.fbx"), model_path("station.fbx") }));
    const resolve_delta delta = resolver.re_resolve(snap, scene_path());

    ASSERT_FALSE(delta.empty());
    EXPECT_TRUE(std::ranges::contains(delta.modified, stable_of(scene_path())));
    EXPECT_TRUE(std::ranges::contains(delta.added, stable_of(model_path("station.fbx"))));
    EXPECT_NE(snap.find(stable_of(model_path("station.fbx"))), nullptr);
  }

  class model_manifest_tests : public asset_resolver_tests {
   protected:
    void SetUp() override {
      asset_resolver_tests::SetUp();
      std::filesystem::create_directories(proj_root / "models");
      std::filesystem::create_directories(proj_root / "textures");
      write_file(proj_root / "textures" / "hull.png", "not a real png");
      write_file(gltf_path(), gltf_with_image_uri("../textures/hull.png"));
    }

    filepath gltf_path() const { return proj_root / "models" / "ship.gltf"; }

    /// carries a buffers[].uri entry on purpose: buffer sidecars must never become nodes
    static std::string gltf_with_image_uri(const std::string_view uri) {
      return std::format(
        R"({{ "asset": {{ "version": "2.0" }}, "images": [ {{ "uri": "{}" }} ], "buffers": [ {{ "byteLength": 4, "uri": "geometry.bin" }} ] }})",
        uri);
    }
  };

  TEST_F(model_manifest_tests, gltf_declares_image_edge_and_skips_buffers) {
    asset_resolver resolver;
    const std::array roots{ gltf_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// gltf + hull.png; geometry.bin stays out of the graph
    ASSERT_EQ(snap.nodes.size(), 2u);
    const dependency_snapshot::node* model_node = snap.find(stable_of(gltf_path()));
    const dependency_snapshot::node* texture_node = snap.find(stable_of(proj_root / "textures" / "hull.png"));
    ASSERT_NE(model_node, nullptr);
    ASSERT_NE(texture_node, nullptr);
    EXPECT_EQ(model_node->type, asset::MODEL_SOURCE);
    EXPECT_EQ(texture_node->type, asset::TEXTURE);

    /// leaves first: [hull.png] -> [ship.gltf]
    ASSERT_EQ(snap.topo_layers.size(), 2u);
    EXPECT_EQ(snap.nodes[snap.topo_layers[1][0]].stable_id, stable_of(gltf_path()));
  }

  TEST_F(model_manifest_tests, gltf_data_uri_images_declare_nothing) {
    write_file(gltf_path(), gltf_with_image_uri("data:image/png;base64,iVBORw0KGgo="));

    asset_resolver resolver;
    const std::array roots{ gltf_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    EXPECT_EQ(snap.nodes.size(), 1u);
  }

  TEST_F(model_manifest_tests, malformed_gltf_declares_nothing) {
    write_file(gltf_path(), "this is { not json");

    asset_resolver resolver;
    const std::array roots{ gltf_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// a broken model file is a data error: warn + no edges, never an assert
    EXPECT_EQ(snap.nodes.size(), 1u);
    EXPECT_NE(snap.find(stable_of(gltf_path())), nullptr);
  }

  class material_manifest_tests : public asset_resolver_tests {
   protected:
    void SetUp() override {
      asset_resolver_tests::SetUp();
      std::filesystem::create_directories(proj_root / "materials");
      std::filesystem::create_directories(proj_root / "textures");
      std::filesystem::create_directories(proj_root / "scenes");
      write_file(proj_root / "textures" / "hull.png", "not a real png");
      write_file(omat_path(), omat_toml("../textures/hull.png"));
    }

    filepath omat_path() const { return proj_root / "materials" / "hull.omat"; }

    static std::string omat_toml(const std::string_view texture_uri) {
      return std::format(
        "asset-type = \"material\"\n"
        "name = \"hull\"\n"
        "\n"
        "[params]\n"
        "base_color = [0.8, 0.85, 0.9, 1.0]\n"
        "roughness = 0.35\n"
        "\n"
        "[textures]\n"
        "base_color = \"{}\"\n",
        texture_uri);
    }
  };

  TEST_F(material_manifest_tests, material_declares_texture_edge) {
    asset_resolver resolver;
    const std::array roots{ omat_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    ASSERT_EQ(snap.nodes.size(), 2u);
    const dependency_snapshot::node* material_node = snap.find(stable_of(omat_path()));
    const dependency_snapshot::node* texture_node = snap.find(stable_of(proj_root / "textures" / "hull.png"));
    ASSERT_NE(material_node, nullptr);
    ASSERT_NE(texture_node, nullptr);
    EXPECT_EQ(material_node->type, asset::MATERIAL);
    EXPECT_EQ(texture_node->type, asset::TEXTURE);

    /// leaves first: [hull.png] -> [hull.omat]
    ASSERT_EQ(snap.topo_layers.size(), 2u);
    EXPECT_EQ(snap.nodes[snap.topo_layers[1][0]].stable_id, stable_of(omat_path()));
  }

  TEST_F(material_manifest_tests, missing_texture_ref_skips_edge) {
    write_file(omat_path(), omat_toml("../textures/does-not-exist.png"));

    asset_resolver resolver;
    const std::array roots{ omat_path() };
    dependency_snapshot snap = resolver.resolve(roots);

    /// dangling texture refs are data errors: warn + skip, the material still resolves
    EXPECT_EQ(snap.nodes.size(), 1u);
    EXPECT_NE(snap.find(stable_of(omat_path())), nullptr);
  }

  TEST_F(material_manifest_tests, scene_material_texture_chain_resolves_in_layers) {
    const filepath scene = proj_root / "scenes" / "chain.oscn";
    write_file(scene, std::format(
                        "[scene]\n"
                        "schema-version = 1\n"
                        "name = \"chain\"\n"
                        "\n"
                        "[[objects]]\n"
                        "id = 1\n"
                        "name = \"Hull\"\n"
                        "\n"
                        "[objects.components.render]\n"
                        "material_asset_id = \"{}\"\n",
                        omat_path().generic_string()));

    asset_resolver resolver;
    const std::array roots{ scene };
    dependency_snapshot snap = resolver.resolve(roots);

    /// scene -> material -> texture with zero new machinery: the codec's asset-ref walk
    /// declares the material, and the material's own parser declares its texture
    ASSERT_EQ(snap.nodes.size(), 3u);
    ASSERT_EQ(snap.topo_layers.size(), 3u);
    EXPECT_EQ(snap.nodes[snap.topo_layers[0][0]].stable_id, stable_of(proj_root / "textures" / "hull.png"));
    EXPECT_EQ(snap.nodes[snap.topo_layers[1][0]].stable_id, stable_of(omat_path()));
    EXPECT_EQ(snap.nodes[snap.topo_layers[2][0]].stable_id, stable_of(scene));
  }

}  // namespace other
