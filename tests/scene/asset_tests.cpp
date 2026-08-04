/**
 * \file tests/scene/asset_tests.cpp
 **/
#include "asset_tests.hpp"

#include <deque>
#include <fstream>

#include <asio/asio.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

#include "file/filesystem.hpp"

#include "gpu_resource/material.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderer_backend.hpp"

#include "serialization/animation_serializer.hpp"

#include "object/animation_component.hpp"
#include "object/render_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver_mounts.hpp"

#include "audio/audio_environment.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
#include "audio/audio_test_fixtures.hpp"
#include "mock_rendering_api.hpp"

namespace other {

  TEST_F(asset_tests, asset_type_from_extension) {
    EXPECT_EQ(asset::get_type_from_extension(".jpg"), asset::TEXTURE);
    EXPECT_EQ(asset::get_type_from_extension(".png"), asset::TEXTURE);
    EXPECT_EQ(asset::get_type_from_extension(".fbx"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".obj"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".gltf"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".glb"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".dae"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".3ds"), asset::MODEL_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".omdl"), asset::MODEL_SOURCE);
    /// the dead .omesh format no longer classifies
    EXPECT_EQ(asset::get_type_from_extension(".omesh"), asset::EMPTY);
    EXPECT_EQ(asset::get_type_from_extension(".oanim"), asset::ANIMATION);
    /// .anim never had a producer or consumer; the row was replaced by .oanim
    EXPECT_EQ(asset::get_type_from_extension(".anim"), asset::EMPTY);
    EXPECT_EQ(asset::get_type_from_extension(".csproj"), asset::SCRIPT_PROJECT);
    EXPECT_EQ(asset::get_type_from_extension(".dll"), asset::SCRIPT_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".so"), asset::SCRIPT_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".cs"), asset::SCRIPT_FILE);
    EXPECT_EQ(asset::get_type_from_extension(".mp3"), asset::AUDIO);
    EXPECT_EQ(asset::get_type_from_extension(".wav"), asset::AUDIO);
    /// scenes are declarative documents; lua files are behavior-hook scripts
    EXPECT_EQ(asset::get_type_from_extension(".lua"), asset::SCRIPT_FILE);
    EXPECT_EQ(asset::get_type_from_extension(".oscn"), asset::SCENE);
    EXPECT_EQ(asset::get_type_from_extension(".oscnb"), asset::SCENE);
    EXPECT_EQ(asset::get_type_from_extension(".os"), asset::SCRIPT);
    EXPECT_EQ(asset::get_type_from_extension(".omat"), asset::MATERIAL);
    EXPECT_EQ(asset::get_type_from_extension(".unknown"), asset::EMPTY);
    /// regression: the extension table once value-initialized a trailing slot, making
    ///  the empty extension classify as TEXTURE (type 0)
    EXPECT_EQ(asset::get_type_from_extension(""), asset::EMPTY);
  }

  TEST_F(asset_tests, supported_extensions) {
    auto texture_exts = asset::get_supported_extensions(asset::TEXTURE);
    EXPECT_NE(std::find(texture_exts.begin(), texture_exts.end(), ".jpg"), texture_exts.end());
    EXPECT_NE(std::find(texture_exts.begin(), texture_exts.end(), ".png"), texture_exts.end());

    auto model_source_exts = asset::get_supported_extensions(asset::MODEL_SOURCE);
    EXPECT_NE(std::find(model_source_exts.begin(), model_source_exts.end(), ".fbx"), model_source_exts.end());
    EXPECT_NE(std::find(model_source_exts.begin(), model_source_exts.end(), ".obj"), model_source_exts.end());

    auto script_project_exts = asset::get_supported_extensions(asset::SCRIPT_PROJECT);
    EXPECT_NE(std::find(script_project_exts.begin(), script_project_exts.end(), ".csproj"), script_project_exts.end());

    auto script_source_exts = asset::get_supported_extensions(asset::SCRIPT_SOURCE);
    EXPECT_NE(std::find(script_source_exts.begin(), script_source_exts.end(), ".dll"), script_source_exts.end());
    EXPECT_NE(std::find(script_source_exts.begin(), script_source_exts.end(), ".so"), script_source_exts.end());

    auto script_file_exts = asset::get_supported_extensions(asset::SCRIPT_FILE);
    EXPECT_NE(std::find(script_file_exts.begin(), script_file_exts.end(), ".cs"), script_file_exts.end());
    EXPECT_NE(std::find(script_file_exts.begin(), script_file_exts.end(), ".lua"), script_file_exts.end());

    auto audio_exts = asset::get_supported_extensions(asset::AUDIO);
    EXPECT_NE(std::find(audio_exts.begin(), audio_exts.end(), ".mp3"), audio_exts.end());
    EXPECT_NE(std::find(audio_exts.begin(), audio_exts.end(), ".wav"), audio_exts.end());

    auto scene_exts = asset::get_supported_extensions(asset::SCENE);
    EXPECT_NE(std::find(scene_exts.begin(), scene_exts.end(), ".oscn"), scene_exts.end());
    EXPECT_NE(std::find(scene_exts.begin(), scene_exts.end(), ".oscnb"), scene_exts.end());
    EXPECT_EQ(std::find(scene_exts.begin(), scene_exts.end(), ".lua"), scene_exts.end());

    auto material_exts = asset::get_supported_extensions(asset::MATERIAL);
    EXPECT_NE(std::find(material_exts.begin(), material_exts.end(), ".omat"), material_exts.end());
    EXPECT_EQ(material_exts.size(), 1u);

    auto animation_exts = asset::get_supported_extensions(asset::ANIMATION);
    EXPECT_NE(std::find(animation_exts.begin(), animation_exts.end(), ".oanim"), animation_exts.end());
    EXPECT_EQ(animation_exts.size(), 1u);
  }

  MATCHER(IsLoadingOrLoaded, "") {
    /// could be any of the following depending on what the computer is doing
    return arg == asset_state::LOADING || arg == asset_state::LOADED;
  }

  namespace {

    void set_up_mock_rendering_api_and_expect_mesh_creation(event_system& events) {
      static mesh test_mesh;
      static gpu_buffer test_vertex_buffer(resource_handle(1, resource_type::BUFFER));
      static gpu_buffer test_index_buffer(resource_handle(2, resource_type::BUFFER));

      using ::testing::_;
      /// first we have to override the rendering subsystem api to avoid nullptr dereference
      scope<mock_rendering_api> mock_api = make_scope<mock_rendering_api>();
      EXPECT_CALL(*mock_api, on_initialize(_)).Times(1);
      // EXPECT_CALL(*mock_api, initialize_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, shutdown_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, on_shutdown(_)).Times(1);

      /// the meaningful load/unload assertions are registry state (get_model_source presence),
      ///  not gpu call counts — exact counts break on every upload refactor. only the two
      ///  structural creates stay counted.
      EXPECT_CALL(*mock_api, create_mesh_resource(_, _))
        .Times(1)
        .WillOnce(testing::Return(&test_mesh));
      EXPECT_CALL(*mock_api, destroy_mesh_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, bind_mesh_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, unbind_mesh_resource(_))
        .Times(testing::AnyNumber());

      EXPECT_CALL(*mock_api, create_buffer_resource(_, _))
        .Times(2)
        .WillOnce(testing::Return(&test_vertex_buffer))
        .WillOnce(testing::Return(&test_index_buffer));
      EXPECT_CALL(*mock_api, destroy_buffer_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, bind_buffer_resource(_, _))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, unbind_buffer_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, set_mesh_vertex_attributes(_, _))
        .Times(testing::AnyNumber());

      EXPECT_CALL(*mock_api, buffer_data(_, _, _, _))
        .Times(testing::AnyNumber());

      subsystem<renderer_backend>::get()->force_set_backend(std::move(mock_api));

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem not available for setting up mock rendering API.");
      fs->initialize_file_events(events);
      constexpr std::array kDefaultMounts = {
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
      };
      fs->initialize_directory_structure(kDefaultMounts);
    }

    void shutdown_mock_rendering_api() {
      subsystem<renderer_backend>::get()->unload_backend();
      subsystem<renderer_backend>::shutdown();
    }

    struct dtor {
      ~dtor() {
        shutdown_mock_rendering_api();
      }
    };

    constexpr static size_t kNumWorkers = 4;
    constexpr static std::string_view kConfig =
      R"(
[application.async]
worker_count = {}
)";

  }  // namespace

  TEST_F(asset_tests, simple_async_load) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);

    set_up_mock_rendering_api_and_expect_mesh_creation(events);

    filepath test_file_path = "tests/resources/models/suzanne3.fbx";
    ASSERT_EQ(std::filesystem::exists(test_file_path), true)
      << "Test asset file does not exist: "
      << test_file_path.string();

    natural_t asset_id = handler->load_asset(test_file_path);
    EXPECT_NE(asset_id, 0);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 1);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_TRUE(handler->asset_exists(asset_id));
    EXPECT_TRUE(handler->asset_loading(asset_id));
    EXPECT_FALSE(handler->asset_loaded(asset_id));

    /// no io-context polling yet so should still be loading
    EXPECT_THAT(handler->get_asset_state(asset_id), asset_state::LOADING);
    std::chrono::seconds load_timeout{ 5 };

    auto start_time = std::chrono::steady_clock::now();
    while (handler->get_asset_state(asset_id) == asset_state::LOADING &&
           std::chrono::steady_clock::now() - start_time < load_timeout) {
      io_context.poll();
      jobs.poll();
      handler->update_pipelines();
    }
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, load_timeout) << "Timed out waiting for asset to load";

    io_context.poll();
    jobs.poll();

    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 1);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_TRUE(handler->asset_exists(asset_id));
    EXPECT_FALSE(handler->asset_loading(asset_id));
    EXPECT_TRUE(handler->asset_loaded(asset_id));

    /// registry state is the load contract: the model source is reachable while loaded
    const natural_t model_hash = handler->get_asset_hash(asset_id);
    EXPECT_NE(subsystem<renderer_backend>::get()->get_model_source(model_hash), nullptr);

    handler->unload_asset(asset_id);
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADING);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 1);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_TRUE(handler->asset_exists(asset_id));
    EXPECT_TRUE(handler->asset_loading(asset_id));
    EXPECT_FALSE(handler->asset_loaded(asset_id));

    start_time = std::chrono::steady_clock::now();
    while (handler->get_asset_state(asset_id) == asset_state::UNLOADING &&
           std::chrono::steady_clock::now() - start_time < load_timeout) {
      io_context.poll();
      jobs.poll();
      handler->update_pipelines();
    }
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, load_timeout) << "Timed out waiting for asset to unload";

    handler->update_pipelines();

    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED)
      << std::format("expected asset {} to be in state UNLOADED, is actually in state: {}", asset_id, handler->get_asset_state(asset_id));
    /// the loaded asset now exists and a pipeline unloading the asset also exists
    ASSERT_EQ(handler->get_num_assets_in_flight(), 0);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_TRUE(handler->asset_exists(asset_id));
    EXPECT_FALSE(handler->asset_loading(asset_id));
    EXPECT_FALSE(handler->asset_loaded(asset_id));

    /// unload must drop the registry entry (and with it the gpu resources)
    EXPECT_EQ(subsystem<renderer_backend>::get()->get_model_source(model_hash), nullptr);

    ASSERT_NO_FATAL_FAILURE(handler->begin_unload());
    EXPECT_EQ(handler->get_num_assets_in_flight(), 0);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);

    handler = nullptr;
  }

  namespace {

    void set_up_mock_rendering_api_for_texture(event_system& events) {
      static texture test_texture;

      using ::testing::_;
      scope<mock_rendering_api> mock_api = make_scope<mock_rendering_api>();
      EXPECT_CALL(*mock_api, on_initialize(_)).Times(1);
      EXPECT_CALL(*mock_api, shutdown_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, on_shutdown(_)).Times(1);

      EXPECT_CALL(*mock_api, create_texture_resource(_, _))
        .Times(1)
        .WillOnce(testing::Return(&test_texture));
      EXPECT_CALL(*mock_api, set_texture_filter(_, _, _)).Times(1);
      EXPECT_CALL(*mock_api, set_texture_wrap_mode(_, _, _, _)).Times(1);
      EXPECT_CALL(*mock_api, upload_texture(_, _, _, _, _, _, _, _, _)).Times(1);
      /// unload_texture must actually destroy the gpu resource
      EXPECT_CALL(*mock_api, destroy_texture_resource(_)).Times(1);

      subsystem<renderer_backend>::get()->force_set_backend(std::move(mock_api));

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem not available for setting up mock rendering API.");
      fs->initialize_file_events(events);
      constexpr std::array kDefaultMounts = {
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
      };
      fs->initialize_directory_structure(kDefaultMounts);
    }

    void set_up_mock_rendering_api_and_expect_no_resource_creation(event_system& events) {
      using ::testing::_;
      scope<mock_rendering_api> mock_api = make_scope<mock_rendering_api>();
      EXPECT_CALL(*mock_api, on_initialize(_)).Times(1);
      EXPECT_CALL(*mock_api, shutdown_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, on_shutdown(_)).Times(1);

      subsystem<renderer_backend>::get()->force_set_backend(std::move(mock_api));

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem not available for setting up mock rendering API.");
      fs->initialize_file_events(events);
      constexpr std::array kDefaultMounts = {
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
      };
      fs->initialize_directory_structure(kDefaultMounts);
    }

    void pump_until(asio::io_context& io_context, job_system& jobs, asset_handler& handler, auto&& done) {
      const std::chrono::seconds timeout{ 5 };
      const auto start_time = std::chrono::steady_clock::now();
      while (!done() && std::chrono::steady_clock::now() - start_time < timeout) {
        io_context.poll();
        jobs.poll();
        handler.update_pipelines();
      }
      ASSERT_TRUE(done()) << "Timed out pumping asset pipelines";
    }

  }  // namespace

  TEST_F(asset_tests, texture_async_load_and_unload) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_for_texture(events);

    filepath test_file_path = "tests/resources/textures/checker4x4.png";
    ASSERT_TRUE(std::filesystem::exists(test_file_path)) << "Test texture does not exist: " << test_file_path.string();

    natural_t asset_id = handler->load_asset(test_file_path);
    ASSERT_NE(asset_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
    ASSERT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);

    /// decoded, uploaded, and registered on the backend under the asset's path hash
    resource_handle texture_handle = subsystem<renderer_backend>::get()->get_texture(handler->get_asset_hash(asset_id));
    EXPECT_NE(texture_handle.id, 0u);
    EXPECT_EQ(texture_handle.type, resource_type::TEXTURE);

    handler->unload_asset(asset_id);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::UNLOADING; });
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    EXPECT_EQ(subsystem<renderer_backend>::get()->get_texture(handler->get_asset_hash(asset_id)).id, 0u);

    handler = nullptr;
  }

  TEST_F(asset_tests, material_async_load_and_unload) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    /// materials are pure cpu parameter sets: no gpu resource is ever created for one
    set_up_mock_rendering_api_and_expect_no_resource_creation(events);

    const filepath mat_dir = std::filesystem::temp_directory_path() / "other-material-asset-tests";
    std::filesystem::remove_all(mat_dir);
    std::filesystem::create_directories(mat_dir);
    const filepath omat = mat_dir / "hull.omat";
    {
      std::ofstream out(omat);
      out << "asset-type = \"material\"\n"
             "name = \"hull\"\n"
             "\n"
             "[params]\n"
             "base_color = [0.8, 0.85, 0.9, 1.0]\n"
             "roughness = 0.35\n";
    }
    /// stable ids virtualize against the mount table, so the temp dir must be mounted
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("materialassets", mat_dir), nullptr);

    natural_t asset_id = handler->load_asset(omat);
    ASSERT_NE(asset_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
    ASSERT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);

    /// parsed and registered on the backend under the asset's path hash
    const natural_t material_hash = handler->get_asset_hash(asset_id);
    const material* mat = subsystem<renderer_backend>::get()->get_material(material_hash);
    ASSERT_NE(mat, nullptr);
    EXPECT_EQ(mat->name, "hull");
    EXPECT_EQ(mat->key, material_hash);
    EXPECT_EQ(mat->revision, 1u);
    ASSERT_EQ(mat->params.size(), 2u);
    EXPECT_FLOAT_EQ(mat->params.at(FNV("roughness")).data.x, 0.35f);

    handler->unload_asset(asset_id);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::UNLOADING; });
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    EXPECT_EQ(subsystem<renderer_backend>::get()->get_material(material_hash), nullptr);

    /// reload = the refresh sequence's unload/load halves; the revision must keep climbing
    /// so pipeline pack caches can never alias a stale blob
    natural_t reloaded_id = handler->load_asset(omat);
    ASSERT_NE(reloaded_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(reloaded_id) != asset_state::LOADING; });
    ASSERT_EQ(handler->get_asset_state(reloaded_id), asset_state::LOADED);
    const material* reloaded = subsystem<renderer_backend>::get()->get_material(material_hash);
    ASSERT_NE(reloaded, nullptr);
    EXPECT_EQ(reloaded->revision, 2u);

    handler->unload_asset(reloaded_id);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(reloaded_id) != asset_state::UNLOADING; });

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(mat_dir, ec);
  }

  TEST_F(asset_tests, animation_async_load_and_unload) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    /// clips are pure cpu keyframe data: no gpu resource is ever created for one
    set_up_mock_rendering_api_and_expect_no_resource_creation(events);

    animation_clip clip;
    clip.name = "walk";
    clip.duration = 1.5f;
    joint_track& track = clip.joint_tracks.emplace_back();
    track.joint_name = "root";
    track.joint_name_hash = FNV("root");
    track.position_keyframes.push_back({ 0.f, glm::vec3(0.f) });
    track.position_keyframes.push_back({ 1.5f, glm::vec3(0.f, 1.f, 0.f) });
    track.rotation_keyframes.push_back({ 0.f, glm::quat(1.f, 0.f, 0.f, 0.f) });

    const filepath anim_dir = std::filesystem::temp_directory_path() / "other-animation-asset-tests";
    std::filesystem::remove_all(anim_dir);
    std::filesystem::create_directories(anim_dir);
    const filepath oanim = anim_dir / "walk.oanim";
    {
      const ostd::vector<uint8_t> bytes = serialization::serialize_animation_clip(clip);
      std::ofstream out(oanim, std::ios::binary);
      out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    /// stable ids virtualize against the mount table, so the temp dir must be mounted
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("animationassets", anim_dir), nullptr);

    natural_t asset_id = handler->load_asset(oanim);
    ASSERT_NE(asset_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
    ASSERT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);

    /// parsed and registered on the backend under the asset's path hash
    const natural_t clip_hash = handler->get_asset_hash(asset_id);
    const animation_clip* loaded = subsystem<renderer_backend>::get()->get_animation(clip_hash);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->name, "walk");
    EXPECT_FLOAT_EQ(loaded->duration, 1.5f);
    ASSERT_EQ(loaded->joint_tracks.size(), 1u);
    EXPECT_EQ(loaded->joint_tracks[0].joint_name_hash, FNV("root"));
    EXPECT_EQ(loaded->joint_tracks[0].position_keyframes.size(), 2u);

    handler->unload_asset(asset_id);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::UNLOADING; });
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    EXPECT_EQ(subsystem<renderer_backend>::get()->get_animation(clip_hash), nullptr);

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(anim_dir, ec);
  }

  TEST_F(asset_tests, backendless_types_load_as_tracked_stubs) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_and_expect_no_resource_creation(events);

    /// input-map has no runtime backend yet; loading must succeed as a tracked
    /// stub, not abort in empty_loader (animation and audio graduated to real loaders)
    const filepath stub_dir = std::filesystem::temp_directory_path() / "other-asset-stub-tests";
    std::filesystem::create_directories(stub_dir);
    const filepath stub_file = stub_dir / "controls.oinputmap";
    {
      std::ofstream out(stub_file);
      out << "stub";
    }
    /// stable ids virtualize against the mount table, so the stub dir must be mounted
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("stubassets", stub_dir), nullptr);

    natural_t asset_id = handler->load_asset(stub_file);
    ASSERT_NE(asset_id, 0) << stub_file.string();
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED) << stub_file.string();

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(stub_dir, ec);
  }

  TEST_F(asset_tests, audio_async_load_unload_reload) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    /// audio clips live on the audio environment: no gpu resource is ever created
    set_up_mock_rendering_api_and_expect_no_resource_creation(events);

    auto* env = subsystem<audio_environment>::get();
    ASSERT_NE(env, nullptr);
    audio_config audio_cfg{};
    audio_cfg.force_pump_mode = true;
    ASSERT_TRUE(env->initialize(audio_cfg));
    struct env_guard {
      ~env_guard() { subsystem<audio_environment>::get()->shutdown(); }
    } ___env_guard;

    const filepath audio_dir = std::filesystem::temp_directory_path() / "other-audio-asset-tests";
    std::filesystem::remove_all(audio_dir);
    std::filesystem::create_directories(audio_dir);
    const filepath wav = audio_dir / "tone.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.25f, 8000, 1));
    {
      std::ofstream sidecar(audio_dir / "tone.wav.odecl.toml");
      sidecar << "schema-version = 1\n"
                 "[audio]\n"
                 "loop = true\n"
                 "volume = 0.5\n";
    }
    /// stable ids virtualize against the mount table, so the temp dir must be mounted
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("audioassets", audio_dir), nullptr);

    natural_t asset_id = handler->load_asset(wav);
    ASSERT_NE(asset_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
    ASSERT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);

    /// decoded on a worker, registered on the environment under the asset's path
    /// hash, with the sidecar folded into clip metadata (never into samples)
    const natural_t clip_hash = handler->get_asset_hash(asset_id);
    const audio_clip* clip = env->get_clip(clip_hash);
    ASSERT_NE(clip, nullptr);
    EXPECT_EQ(clip->channels, 1u);
    EXPECT_EQ(clip->sample_rate, 8000u);
    EXPECT_EQ(clip->frames, 2000u);
    EXPECT_EQ(clip->pcm.size(), 2000u);
    EXPECT_TRUE(clip->default_loop);
    EXPECT_FLOAT_EQ(clip->default_gain, 0.5f);
    EXPECT_EQ(env->clip_revision(clip_hash), 1u);

    /// disk edit -> refresh runs unload-then-load; the revision bumps so bound
    /// voices can detect the swap
    ASSERT_TRUE(write_test_wav(wav, 0.5f, 8000, 1));
    handler->reload_asset(asset_id);
    pump_until(io_context, jobs, *handler, [&] {
      return handler->get_asset_state(asset_id) == asset_state::LOADED && env->clip_revision(clip_hash) == 2u;
    });
    clip = env->get_clip(clip_hash);
    ASSERT_NE(clip, nullptr);
    EXPECT_EQ(clip->frames, 4000u);

    handler->unload_asset(asset_id);
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::UNLOADING; });
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    EXPECT_EQ(env->get_clip(clip_hash), nullptr);
    /// high-water: revision survives remove
    EXPECT_EQ(env->clip_revision(clip_hash), 2u);

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(audio_dir, ec);
  }

  TEST_F(asset_tests, unload_requested_while_loading_defers_and_drains) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_and_expect_mesh_creation(events);

    filepath test_file_path = "tests/resources/models/suzanne3.fbx";
    ASSERT_TRUE(std::filesystem::exists(test_file_path));

    natural_t asset_id = handler->load_asset(test_file_path);
    ASSERT_NE(asset_id, 0);
    ASSERT_EQ(handler->get_asset_state(asset_id), asset_state::LOADING);

    /// loading a path already in flight folds into the existing pipeline
    EXPECT_EQ(handler->load_asset(test_file_path), asset_id);
    EXPECT_EQ(handler->get_num_loading_assets(), 1);

    /// unload during load defers; the load must not be torn down mid-flight
    handler->unload_asset(asset_id);
    EXPECT_EQ(handler->get_num_pending_unloads(), 1);
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::LOADING);

    /// once the load settles, update_pipelines drains the deferred unload to completion
    pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) == asset_state::UNLOADED; });
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_FALSE(handler->asset_loaded(asset_id));
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);

    handler = nullptr;
  }

  /// asset machinery + the script environment: prepare_render_data tests need live scenes
  ///  (script_component hooks) AND models loaded through the real pipeline
  class animation_draw_tests : public asset_tests {
   protected:
    bool script_and_physics() const override { return true; }
  };

  namespace {

    /// mock setup for tests uploading a known number of model sources (each upload = one
    ///  mesh create + a vertex/index buffer pair, in that order). every resource is a
    ///  FRESH object carrying its minted handle, like the real backend: mesh_key batching,
    ///  destroy_model, and the per-object reference counts all depend on per-resource
    ///  identity (a shared static aliases the counts and trips the decrement assert)
    void set_up_mock_rendering_api_for_model_uploads(event_system& events, int model_count) {
      using ::testing::_;
      scope<mock_rendering_api> mock_api = make_scope<mock_rendering_api>();
      EXPECT_CALL(*mock_api, on_initialize(_)).Times(1);
      EXPECT_CALL(*mock_api, shutdown_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, on_shutdown(_)).Times(1);

      EXPECT_CALL(*mock_api, create_mesh_resource(_, _))
        .Times(model_count)
        .WillRepeatedly(testing::Invoke([](const resource_handle& handle, resource_type) -> mesh* {
          static std::deque<mesh> meshes;  /// stable addresses; callers hold the pointer
          return &meshes.emplace_back(handle);
        }));
      EXPECT_CALL(*mock_api, destroy_mesh_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, bind_mesh_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, unbind_mesh_resource(_))
        .Times(testing::AnyNumber());

      EXPECT_CALL(*mock_api, create_buffer_resource(_, _))
        .Times(2 * model_count)
        .WillRepeatedly(testing::Invoke([](const resource_handle& handle, resource_type) -> gpu_buffer* {
          static std::deque<gpu_buffer> buffers;  /// stable addresses; callers hold the pointer
          return &buffers.emplace_back(handle);
        }));
      EXPECT_CALL(*mock_api, destroy_buffer_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, bind_buffer_resource(_, _))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, unbind_buffer_resource(_))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, set_mesh_vertex_attributes(_, _))
        .Times(testing::AnyNumber());
      EXPECT_CALL(*mock_api, buffer_data(_, _, _, _))
        .Times(testing::AnyNumber());

      subsystem<renderer_backend>::get()->force_set_backend(std::move(mock_api));

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem not available for setting up mock rendering API.");
      fs->initialize_file_events(events);
      constexpr std::array kDefaultMounts = {
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
      };
      fs->initialize_directory_structure(kDefaultMounts);
    }

    const gpu::bone_matrix_buffer* find_draw_buffer(const render_data& data, resource_handle source_handle) {
      for (size_t i = 0; i < data.mesh_keys.size(); ++i) {
        if (data.mesh_keys[i].model_source_handle.id == source_handle.id) {
          return &data.bone_buffers[i];
        }
      }
      return nullptr;
    }

  }  // namespace

  /// each draw's bone buffer holds its own entity's palette, non-destructively, with
  ///  use_bones set only for rigged draws
  TEST_F(animation_draw_tests, bone_buffers_per_draw) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_for_model_uploads(events, 3);

    /// two copies of the rigged glb = two distinct assets = two distinct draws; one static model
    const filepath rig_dir = std::filesystem::temp_directory_path() / "other-bone-draw-tests";
    std::filesystem::remove_all(rig_dir);
    std::filesystem::create_directories(rig_dir);
    std::filesystem::copy_file("tests/resources/models/bone-test-2-1.glb", rig_dir / "rig_a.glb");
    std::filesystem::copy_file("tests/resources/models/bone-test-2-1.glb", rig_dir / "rig_b.glb");
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("bonedrawassets", rig_dir), nullptr);

    const filepath rig_a_path = rig_dir / "rig_a.glb";
    const filepath rig_b_path = rig_dir / "rig_b.glb";
    const filepath static_path = "tests/resources/models/suzanne3.fbx";
    const natural_t rig_a_id = handler->load_asset(rig_a_path);
    const natural_t rig_b_id = handler->load_asset(rig_b_path);
    const natural_t static_id = handler->load_asset(static_path);
    ASSERT_NE(rig_a_id, 0);
    ASSERT_NE(rig_b_id, 0);
    ASSERT_NE(static_id, 0);
    pump_until(io_context, jobs, *handler, [&] {
      return handler->asset_loaded(rig_a_id) && handler->asset_loaded(rig_b_id) && handler->asset_loaded(static_id);
    });

    scene s("Bone Draw Scene");
    const auto add_render_object = [&s](const char* name, natural_t asset_id) -> natural_t {
      scene_object& obj = s.create_object(name);
      render_component render = {};
      render.model_asset_id = asset_id;
      s.add_component<render_component>(&obj, std::move(render));
      return obj.id;
    };
    const natural_t rig_a_obj = add_render_object("RigA", rig_a_id);
    const natural_t rig_b_obj = add_render_object("RigB", rig_b_id);
    add_render_object("Static", static_id);

    /// first prepare produces the model instances; no palettes exist yet
    render_data first = s.prepare_render_data(handler);
    ASSERT_EQ(first.draw_calls.size(), 5u);  /// 1 + 1 rigged submeshes + 3 static submeshes
    for (const gpu::bone_matrix_buffer& bone_buff : first.bone_buffers) {
      EXPECT_EQ(bone_buff.use_bones, 0);
    }

    /// distinct palettes, written the way the animation tick writes them
    render_component* rig_a_render = s.try_get_component<render_component>(rig_a_obj);
    render_component* rig_b_render = s.try_get_component<render_component>(rig_b_obj);
    ASSERT_NE(rig_a_render, nullptr);
    ASSERT_NE(rig_b_render, nullptr);
    rig_a_render->obj_model.bone_matrices = {
      glm::translate(glm::mat4(1.f), glm::vec3(1.f, 0.f, 0.f)),
      glm::translate(glm::mat4(1.f), glm::vec3(2.f, 0.f, 0.f)),
    };
    rig_b_render->obj_model.bone_matrices = {
      glm::translate(glm::mat4(1.f), glm::vec3(0.f, 3.f, 0.f)),
      glm::translate(glm::mat4(1.f), glm::vec3(0.f, 4.f, 0.f)),
    };

    render_data data = s.prepare_render_data(handler);
    ASSERT_EQ(data.draw_calls.size(), 5u);

    renderer_backend* backend = subsystem<renderer_backend>::get();
    const gpu::bone_matrix_buffer* rig_a_buffer = find_draw_buffer(data, backend->get_model_source(handler->get_asset_hash(rig_a_id))->get_mesh_handle());
    const gpu::bone_matrix_buffer* rig_b_buffer = find_draw_buffer(data, backend->get_model_source(handler->get_asset_hash(rig_b_id))->get_mesh_handle());
    const gpu::bone_matrix_buffer* static_buffer = find_draw_buffer(data, backend->get_model_source(handler->get_asset_hash(static_id))->get_mesh_handle());
    ASSERT_NE(rig_a_buffer, nullptr);
    ASSERT_NE(rig_b_buffer, nullptr);
    ASSERT_NE(static_buffer, nullptr);

    /// each draw holds ITS OWN palette
    EXPECT_EQ(rig_a_buffer->use_bones, 1);
    EXPECT_EQ(rig_a_buffer->bone_matrices[0], rig_a_render->obj_model.bone_matrices[0]);
    EXPECT_EQ(rig_a_buffer->bone_matrices[1], rig_a_render->obj_model.bone_matrices[1]);
    EXPECT_EQ(rig_b_buffer->use_bones, 1);
    EXPECT_EQ(rig_b_buffer->bone_matrices[0], rig_b_render->obj_model.bone_matrices[0]);
    EXPECT_EQ(rig_b_buffer->bone_matrices[1], rig_b_render->obj_model.bone_matrices[1]);
    EXPECT_NE(rig_a_buffer->bone_matrices[0], rig_b_buffer->bone_matrices[0]);

    /// the static draw is untouched by every other entity's palette
    EXPECT_EQ(static_buffer->use_bones, 0);

    /// the fill is NOT destructive: the palette survives for the next frame's prepare
    EXPECT_EQ(rig_a_render->obj_model.bone_matrices.size(), 2u);

    /// a live palette drives the object's AABB: union of each joint's bind-space
    ///  influenced bounds through its palette matrix, then the world transform —
    ///  the box follows the animation instead of freezing at the bind pose
    {
      const model_data& src = rig_a_render->obj_model.source->source_data();
      bounding_box expected = bounding_box::empty;
      for (size_t i = 0; i < src.skel.joints.size(); ++i) {
        bounding_box joint_bounds = src.skel.joints[i].influenced_bounds;
        if (joint_bounds == bounding_box::empty) {
          continue;
        }
        expected = bounding_box::expand_to_include(expected, joint_bounds.transform(rig_a_render->obj_model.bone_matrices[i]));
      }
      ASSERT_FALSE(expected == bounding_box::empty);
      expected = expected.transform(s.get_world_transform(rig_a_obj));

      const bounding_box animated = s.get_bounding_box(rig_a_obj);
      EXPECT_FLOAT_EQ(animated.min.x, expected.min.x);
      EXPECT_FLOAT_EQ(animated.min.y, expected.min.y);
      EXPECT_FLOAT_EQ(animated.min.z, expected.min.z);
      EXPECT_FLOAT_EQ(animated.max.x, expected.max.x);
      EXPECT_FLOAT_EQ(animated.max.y, expected.max.y);
      EXPECT_FLOAT_EQ(animated.max.z, expected.max.z);

      /// and it is genuinely different from the static bind bounds (the palettes translate)
      bounding_box static_box = src.bounds;
      static_box = static_box.transform(s.get_world_transform(rig_a_obj));
      EXPECT_FALSE(animated == static_box);
    }

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(rig_dir, ec);
  }

  /// tick and draw fill composed end-to-end: embedded clip resolved from the loaded glb,
  ///  sampled by scene::update, landing in the draw's bone buffer — and the play/stop
  ///  snapshot restores the pre-play clock
  TEST_F(animation_draw_tests, animation_tick_feeds_draw_palette) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_for_model_uploads(events, 1);

    const filepath rig_path = "tests/resources/models/bone-test-2-1.glb";
    const natural_t rig_id = handler->load_asset(rig_path);
    ASSERT_NE(rig_id, 0);
    pump_until(io_context, jobs, *handler, [&] { return handler->asset_loaded(rig_id); });

    scene s("Tick Draw Scene");
    scene_object& dancer = s.create_object("Dancer");
    {
      render_component render = {};
      render.model_asset_id = rig_id;
      s.add_component<render_component>(&dancer, std::move(render));

      animation_component anim = {};
      anim.clip_name = "ArmatureAction";
      anim.time = 0.4f;
      s.add_component<animation_component>(&dancer, std::move(anim));
    }

    /// first prepare produces the model; the tick needs obj_model.source resolved
    render_data first = s.prepare_render_data(handler);
    ASSERT_EQ(first.draw_calls.size(), 1u);
    EXPECT_EQ(first.bone_buffers[0].use_bones, 0);

    s.play();
    s.update(0.1, handler);

    render_component* render = s.try_get_component<render_component>(s.find_object(std::string_view{ "Dancer" })->id);
    animation_component* anim = s.try_get_component<animation_component>(s.find_object(std::string_view{ "Dancer" })->id);
    ASSERT_NE(render, nullptr);
    ASSERT_NE(anim, nullptr);

    /// the tick resolved the embedded clip, advanced the clock, and built the palette
    ASSERT_NE(anim->clip, nullptr);
    EXPECT_EQ(anim->clip->name, "ArmatureAction");
    EXPECT_FLOAT_EQ(anim->time, 0.5f);
    ASSERT_EQ(render->obj_model.bone_matrices.size(), 2u);

    render_data data = s.prepare_render_data(handler);
    ASSERT_EQ(data.draw_calls.size(), 1u);
    EXPECT_EQ(data.bone_buffers[0].use_bones, 1);
    EXPECT_EQ(data.bone_buffers[0].bone_matrices[0], render->obj_model.bone_matrices[0]);
    EXPECT_EQ(data.bone_buffers[0].bone_matrices[1], render->obj_model.bone_matrices[1]);

    /// stop's restore rewinds the clock to the pre-play serialized time; runtime state
    ///  is rebuilt (not resurrected) by the next tick
    s.stop();
    animation_component* restored = s.try_get_component<animation_component>(s.find_object(std::string_view{ "Dancer" })->id);
    ASSERT_NE(restored, nullptr);
    EXPECT_FLOAT_EQ(restored->time, 0.4f);
    EXPECT_EQ(restored->clip, nullptr);

    handler = nullptr;
  }

}  // namespace other