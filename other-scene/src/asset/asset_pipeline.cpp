/**
 * \file asset/asset_pipeline.cpp
 **/
#include "asset/asset_pipeline.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
#include "asset/model_source_pipeline.hpp"

namespace other {

  namespace detail {

    void load_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void load_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void load_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void load_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void load_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void empty_loader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);

    void unload_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void unload_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void unload_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void unload_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void unload_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);
    void empty_unloader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline);

  }  // namespace detail

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::loaders = {
    detail::load_texture,
    detail::load_model_source,
    detail::load_model,
    detail::load_script_source,
    detail::load_audio,
    detail::empty_loader,
  };

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::unloaders = {
    detail::unload_texture,
    detail::unload_model_source,
    detail::unload_model,
    detail::unload_script_source,
    detail::unload_audio,
    detail::empty_unloader,
  };

  scope<asset_pipeline> asset_pipeline::get_asset_pipeline(asset::type type, asset_handler* handler) {
    switch (type) {
      case asset::MODEL_SOURCE: return make_scope<model_source_pipeline>(handler);
      default:
        OTHER_ASSERT(false, "No asset pipeline for asset type {}", type);
    }
  }

  void asset_pipeline::start_load(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    CORE_LOG_DEBUG("Starting load pipeline for asset ID: {}", asset_ptr->id);

    pipeline_state.loading = true;
    start_load_operation(
      execution_pool, asset_ptr,
      on_success, on_failure,
      loading_table::loaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::start_unload(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    CORE_LOG_DEBUG("Starting unload pipeline for asset ID: {}", asset_ptr->id);

    pipeline_state.unloading = true;
    start_load_operation(
      execution_pool, asset_ptr,
      on_success, on_failure,
      loading_table::unloaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::poll() {
    if (on_success_callback == nullptr || on_failure_callback == nullptr) {
      return;
    }

    on_pipeline_poll();
    if (pipeline_state.success) {
      pipeline_complete(asset_ptr);
    } else if (pipeline_state.failure) {
      pipeline_failed(asset_ptr, error_message);
    }

    if (pipeline_state.success || pipeline_state.failure) {
      reset();
    }
  }

  void asset_pipeline::start_load_operation(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, loading_table::loader_fn_t function) {
    on_success_callback = on_success;
    on_failure_callback = on_failure;
    this->asset_ptr = asset_ptr;

    CORE_LOG_TRACE("Posting load operation for asset ID: {} to execution pool", asset_ptr->id);
    asio::post(execution_pool, [this, asset_ptr, function]() {
      OTHER_ASSERT(function != nullptr, "load operation function is null for asset type {}", static_cast<size_t>(asset_ptr->asset_type));
      try {
        function(
          asset_ptr,
          [this]() { pipeline_finished(); },
          [this](const std::string& error_msg) { pipeline_failed(error_msg); },
          this
        );
        CORE_LOG_TRACE("Load operation for asset ID: {} completed", asset_ptr->id);
      } catch (const std::exception& e) {
        pipeline_failed(e.what());
      } catch (...) {
        pipeline_failed("Unknown Error");
      }
    });
  }

  void asset_pipeline::pipeline_finished() {
    pipeline_state.success = true;
    CORE_LOG_TRACE("    pipeline_state.success = {}", (bool)pipeline_state.success);
  }

  void asset_pipeline::pipeline_failed(const std::string& error_message) {
    CORE_LOG_TRACE("    pipeline_state.failure = {}", (bool)pipeline_state.failure);
    {
      std::lock_guard lck{ mtx };
      this->error_message = error_message;
    }
    pipeline_state.failure = true;
  }

  void asset_pipeline::reset() {
    pipeline_state.success = false;
    pipeline_state.failure = false;

    pipeline_state.loading = false;
    pipeline_state.unloading = false;
    error_message = "";
  }

  void asset_pipeline::pipeline_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in pipeline_complete");
    OTHER_ASSERT(on_success_callback != nullptr, "on_success_callback is null in pipeline_complete");
    CORE_LOG_TRACE("Pipeline complete for asset ID: {}", asset_ptr->id);

    if (pipeline_state.loading) {
      on_load_complete(asset_ptr);
    } else if (pipeline_state.unloading) {
      on_unload_complete(asset_ptr);
    }
    on_success_callback();
  }

  void asset_pipeline::pipeline_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(on_failure_callback != nullptr, "on_failure_callback is null in pipeline_failed");
    CORE_LOG_TRACE("Pipeline failed for asset ID: {} with error: {}", asset_ptr->id, error_message);

    if (pipeline_state.loading) {
      on_load_failed(asset_ptr, error_message);
    } else if (pipeline_state.unloading) {
      on_unload_failed(asset_ptr, error_message);
    }
    on_failure_callback(error_message);
  }

  namespace detail {

    void load_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void load_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null");
      OTHER_ASSERT(on_success != nullptr, "on_success callback is null");
      OTHER_ASSERT(on_failure != nullptr, "on_failure callback is null");

      model_source_pipeline* pl = reinterpret_cast<model_source_pipeline*>(pipeline);
      OTHER_ASSERT(pl != nullptr, "Pipeline is null!");

      /// we've already confirmed this existence at this point
      OTHER_ASSERT(std::filesystem::exists(asset_ptr->path), "Model source file does not exist: {}", asset_ptr->path.string());

      filepath file_path = asset_ptr->path;
      CORE_LOG_DEBUG("Loading model source from file: {}", file_path.string());

      model_builder builder = model_importer::load_model_data(file_path);
      CORE_LOG_DEBUG("Model source loaded");
      if (builder.vertices.empty() || builder.indices.empty() ||
          builder.submeshes.empty() || builder.nodes.empty()) {
        on_failure(std::format("Failed to load model source: {} (data invalid)", file_path.string()));
      } else {
        on_success();
        {
          std::lock_guard lck{ pl->mtx };
          pl->builder = std::move(builder);
        }
      }
    }

    void load_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void load_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void load_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void empty_loader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
      on_success();
    }

    void unload_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void unload_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
      CORE_LOG_DEBUG("Unloading model source (ID: {})", asset_ptr->id);

      /// nothing to do here for now since renderer_backend handles it,
      ///  later we will want to check if there is anything that needs to be written to disk, etc.
      /// most of the work has to happen in the renderer_backend on the rendering thread
      on_success();
    }

    void unload_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void unload_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void unload_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
    }

    void empty_unloader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure, void* pipeline) {
      on_success();
    }

  }  // namespace detail
}  // namespace other