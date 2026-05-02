/**
 * \file tests/scene/asset_tests.cpp
 **/
#include "asset_tests.hpp"

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
    EXPECT_EQ(asset::get_type_from_extension(".cs"), asset::SCRIPT_SOURCE);
    EXPECT_EQ(asset::get_type_from_extension(".dll"), asset::SCRIPT);
    EXPECT_EQ(asset::get_type_from_extension(".so"), asset::SCRIPT);
    EXPECT_EQ(asset::get_type_from_extension(".lua"), asset::SCENE);
    EXPECT_EQ(asset::get_type_from_extension(".py"), asset::SCRIPT);
    EXPECT_EQ(asset::get_type_from_extension(".mp3"), asset::AUDIO);
    EXPECT_EQ(asset::get_type_from_extension(".wav"), asset::AUDIO);
    EXPECT_EQ(asset::get_type_from_extension(".lua"), asset::SCENE);
    EXPECT_EQ(asset::get_type_from_extension(".unknown"), asset::EMPTY);
  }

  TEST_F(asset_tests, supported_extensions) {
    auto texture_exts = asset::get_supported_extensions(asset::TEXTURE);
    EXPECT_NE(std::find(texture_exts.begin(), texture_exts.end(), ".jpg"), texture_exts.end());
    EXPECT_NE(std::find(texture_exts.begin(), texture_exts.end(), ".png"), texture_exts.end());

    auto model_source_exts = asset::get_supported_extensions(asset::MODEL_SOURCE);
    EXPECT_NE(std::find(model_source_exts.begin(), model_source_exts.end(), ".fbx"), model_source_exts.end());
    EXPECT_NE(std::find(model_source_exts.begin(), model_source_exts.end(), ".obj"), model_source_exts.end());

    auto script_source_exts = asset::get_supported_extensions(asset::SCRIPT_SOURCE);
    EXPECT_NE(std::find(script_source_exts.begin(), script_source_exts.end(), ".cs"), script_source_exts.end());

    auto script_exts = asset::get_supported_extensions(asset::SCRIPT);
    EXPECT_NE(std::find(script_exts.begin(), script_exts.end(), ".dll"), script_exts.end());
    EXPECT_NE(std::find(script_exts.begin(), script_exts.end(), ".so"), script_exts.end());
    EXPECT_NE(std::find(script_exts.begin(), script_exts.end(), ".py"), script_exts.end());

    auto audio_exts = asset::get_supported_extensions(asset::AUDIO);
    EXPECT_NE(std::find(audio_exts.begin(), audio_exts.end(), ".mp3"), audio_exts.end());
    EXPECT_NE(std::find(audio_exts.begin(), audio_exts.end(), ".wav"), audio_exts.end());

    auto scene_exts = asset::get_supported_extensions(asset::SCENE);
    EXPECT_NE(std::find(scene_exts.begin(), scene_exts.end(), ".lua"), scene_exts.end());
  }

  MATCHER(IsLoadingOrLoaded, "") {
    /// could be any of the following depending on what the computer is doing
    return arg == asset_state::LOADING || arg == asset_state::LOADED;
  }

  namespace {

    static mesh test_mesh;
    static gpu_buffer test_vertex_buffer(resource_handle(1, resource_type::BUFFER));
    static gpu_buffer test_index_buffer(resource_handle(2, resource_type::BUFFER));

    void set_up_mock_rendering_api_and_expect_mesh_creation(event_system& events) {
      using ::testing::_;
      /// first we have to override the rendering subsystem api to avoid nullptr dereference
      scope<mock_rendering_api> mock_api = make_scope<mock_rendering_api>();
      EXPECT_CALL(*mock_api, on_initialize(_)).Times(1);
      // EXPECT_CALL(*mock_api, initialize_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, shutdown_ui_context()).Times(1);
      EXPECT_CALL(*mock_api, on_shutdown(_)).Times(1);

      EXPECT_CALL(*mock_api, create_mesh_resource(_, _))
        .Times(1)
        .WillOnce(testing::Return(&test_mesh));
      // EXPECT_CALL(*mock_api, destroy_mesh_resource(_))
      //   .Times(1);
      EXPECT_CALL(*mock_api, bind_mesh_resource(_))
        .Times(3);
      EXPECT_CALL(*mock_api, unbind_mesh_resource(_))
        .Times(3);

      EXPECT_CALL(*mock_api, create_buffer_resource(_, _))
        .Times(2)
        .WillOnce(testing::Return(&test_vertex_buffer))
        .WillOnce(testing::Return(&test_index_buffer));
      // EXPECT_CALL(*mock_api, destroy_buffer_resource(_))
      //   .Times(2);
      EXPECT_CALL(*mock_api, bind_buffer_resource(_, _))
        .Times(2);
      EXPECT_CALL(*mock_api, unbind_buffer_resource(_))
        .Times(2);
      EXPECT_CALL(*mock_api, set_mesh_vertex_attributes(_, _))
        .Times(1);

      EXPECT_CALL(*mock_api, buffer_data(_, _, _, _))
        .Times(2);

      subsystem<renderer_backend>::get()->force_set_backend(std::move(mock_api));

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem not available for setting up mock rendering API.");
      fs->initialize_file_events(events);
      fs->initialize_directory_structure({
        driver_mounts::kAssetMount,
        driver_mounts::kSceneMount,
        driver_mounts::kScriptMount,
      });
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

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    jobs.initialize(cfg);

    scope<asset_handler> handler = make_scope<asset_handler>(events, io_context, jobs);

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

    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    /// the loaded asset now exists and a pipeline unloading the asset also exists
    ASSERT_EQ(handler->get_num_assets_in_flight(), 0);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);
    EXPECT_TRUE(handler->asset_exists(asset_id));
    EXPECT_FALSE(handler->asset_loading(asset_id));
    EXPECT_FALSE(handler->asset_loaded(asset_id));

    ASSERT_NO_FATAL_FAILURE(handler->begin_unload());
    EXPECT_EQ(handler->get_num_assets_in_flight(), 0);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);

    handler = nullptr;
  }

  TEST_F(asset_tests, omesh_async_load) {
    GTEST_SKIP() << "Skipping omesh test until we have a way to generate them in CI, files are too large to push to git (may have to use github lfs?)";

    job_system jobs{ io_context };
    scope<asset_handler> handler = make_scope<asset_handler>(events, io_context, jobs);

    set_up_mock_rendering_api_and_expect_mesh_creation(events);

    struct dtor {
      ~dtor() {
        shutdown_mock_rendering_api();
      }
    } ___destructor_guard;

    filepath test_file_path = "tests/resources/models/NewSponza_Curtains_FBX_YUp_fbx7binary.omesh";
    ASSERT_EQ(std::filesystem::exists(test_file_path), true)
      << "Test asset file does not exist: "
      << test_file_path.string();

    natural_t asset_id = handler->load_asset(test_file_path);
    EXPECT_NE(asset_id, 0);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 1);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);

    /// no io-context polling yet so should still be loading
    EXPECT_THAT(handler->get_asset_state(asset_id), asset_state::LOADING);

    std::chrono::seconds load_timeout{ 10 };

    auto start_time = std::chrono::steady_clock::now();
    while (handler->get_asset_state(asset_id) == asset_state::LOADING &&
           std::chrono::steady_clock::now() - start_time < load_timeout) {
      io_context.poll();
      handler->update_pipelines();
    }
    std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time);
    ASSERT_LT(duration, load_timeout) << "Timed out waiting for asset to load";

    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::LOADED);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 1);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);

    handler->unload_asset(asset_id);
    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADING);

    start_time = std::chrono::steady_clock::now();
    while (handler->get_asset_state(asset_id) == asset_state::UNLOADING &&
           std::chrono::steady_clock::now() - start_time < load_timeout) {
      io_context.poll();
      handler->update_pipelines();
    }
    duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time);
    ASSERT_LT(duration, load_timeout) << "Timed out waiting for asset to unload";

    EXPECT_EQ(handler->get_asset_state(asset_id), asset_state::UNLOADED);
    ASSERT_EQ(handler->get_num_assets_in_flight(), 1);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 1);
    EXPECT_EQ(handler->get_num_pending_unloads(), 1);

    ASSERT_NO_FATAL_FAILURE(handler->begin_unload());
    EXPECT_EQ(handler->get_num_assets_in_flight(), 0);
    EXPECT_EQ(handler->get_num_loading_assets(), 0);
    EXPECT_EQ(handler->get_num_loaded_assets(), 0);
    EXPECT_EQ(handler->get_num_pending_unloads(), 0);

    handler = nullptr;
  }

}  // namespace other