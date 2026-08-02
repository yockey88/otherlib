/**
 * \file tests/scene/asset_tests.cpp
 **/
#include "asset_tests.hpp"

#include <fstream>

#include <asio/asio.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "file/filesystem.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/renderer_backend.hpp"

#include "driver/driver_mounts.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
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

  TEST_F(asset_tests, backendless_types_load_as_tracked_stubs) {
    dtor ___destructor_guard;

    job_system jobs{ io_context };
    event_system events{ io_context };
    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, jobs);
    set_up_mock_rendering_api_and_expect_no_resource_creation(events);

    /// audio/animation/input-map have no runtime backend yet; loading must succeed
    /// as a tracked stub, not abort in empty_loader
    const filepath stub_dir = std::filesystem::temp_directory_path() / "other-asset-stub-tests";
    std::filesystem::create_directories(stub_dir);
    const std::array stub_files = {
      stub_dir / "tone.wav",
      stub_dir / "walk.anim",
      stub_dir / "controls.oinputmap",
    };
    for (const filepath& file : stub_files) {
      std::ofstream out(file);
      out << "stub";
    }
    /// stable ids virtualize against the mount table, so the stub dir must be mounted
    ASSERT_NE(subsystem<file_system>::get()->mount_directory("stubassets", stub_dir), nullptr);

    for (const filepath& file : stub_files) {
      natural_t asset_id = handler->load_asset(file);
      ASSERT_NE(asset_id, 0) << file.string();
      pump_until(io_context, jobs, *handler, [&] { return handler->get_asset_state(asset_id) != asset_state::LOADING; });
      EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED) << file.string();
    }

    handler = nullptr;
    std::error_code ec;
    std::filesystem::remove_all(stub_dir, ec);
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

}  // namespace other