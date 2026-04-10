/**
 * \file asset/asset_pipeline.cpp
 **/
#include "asset/asset_pipeline.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
#include "asset/model_source_pipeline.hpp"

namespace other {

  namespace detail {

    void load_texture(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void load_model_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void load_model(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void load_script_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void load_audio(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void empty_loader(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

    void unload_texture(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void unload_model_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void unload_model(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void unload_script_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void unload_audio(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    void empty_unloader(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

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

  void asset_pipeline::start_load(asio::thread_pool& execution_pool, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
    CORE_LOG_DEBUG("Starting load pipeline for asset ID: {}", asset_ptr->id);

    if (pipeline_state.loading) {
      CORE_LOG_WARN("Pipeline is already loading for asset ID: {}", asset_ptr->id);
      return;
    }

    pipeline_state.loading = true;
    start_load_operation(
      execution_pool, asset_ptr, on_success, on_failure,
      loading_table::loaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::start_unload(asio::thread_pool& execution_pool, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
    CORE_LOG_DEBUG("Starting unload pipeline for asset ID: {}", asset_ptr->id);

    if (pipeline_state.unloading) {
      CORE_LOG_WARN("Pipeline is already unloading for asset ID: {}", asset_ptr->id);
      return;
    }
    /// \todo add a flag to immediately unload after loading completes
    else if (pipeline_state.loading) {
      CORE_LOG_WARN("Pipeline is currently loading for asset ID: {}. Cannot unload while loading.", asset_ptr->id);
      return;
    }

    pipeline_state.unloading = true;
    start_load_operation(
      execution_pool, asset_ptr, on_success, on_failure,
      loading_table::unloaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::poll() {
    OTHER_ASSERT(on_success_callback != nullptr, "on_success_callback is null in poll");
    OTHER_ASSERT(on_failure_callback != nullptr, "on_failure_callback is null in poll");
    if (!pipeline_state.loading && !pipeline_state.unloading) {
      return;
    }

    on_pipeline_poll();
    if (pipeline_state.success) {
      std::lock_guard lck{ mtx };
      pipeline_complete(asset_ptr);
    } else if (pipeline_state.failure) {
      std::string error_message;
      pipeline_failed(asset_ptr, error_message);
    }

    if (pipeline_state.success || pipeline_state.failure) {
      reset();
    }
  }

  void asset_pipeline::start_load_operation(asio::thread_pool& execution_pool, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure, loading_table::loader_fn_t function) {
    on_success_callback = on_success;
    on_failure_callback = on_failure;
    this->asset_ptr = asset_ptr;

    CORE_LOG_TRACE("Posting load operation for asset ID: {} to execution pool", asset_ptr->id);
    asio::post(execution_pool, [this, asset_ptr, function]() {
      OTHER_ASSERT(function != nullptr, "load operation function is null for asset type {}", static_cast<size_t>(asset_ptr->asset_type));
      try {
        function(asset_ptr, &asset_pipeline::pipeline_finished, &asset_pipeline::pipeline_failed, this);
      } catch (const std::exception& e) {
        pipeline_failed(e.what());
      } catch (...) {
        pipeline_failed("Unknown Error");
      }
    });
  }

  void asset_pipeline::pipeline_finished() {
    CORE_LOG_DEBUG("Pipeline finished successfully");
    pipeline_state.success = true;
  }

  void asset_pipeline::pipeline_failed(const std::string& error_message) {
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

    if (on_success_callback != nullptr) {
      on_success_callback(asset_ptr);
    }
  }

  void asset_pipeline::pipeline_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(on_failure_callback != nullptr, "on_failure_callback is null in pipeline_failed");
    CORE_LOG_TRACE("Pipeline failed for asset ID: {} with error: {}", asset_ptr->id, error_message);

    if (pipeline_state.loading) {
      on_load_failed(asset_ptr, error_message);
    } else if (pipeline_state.unloading) {
      on_unload_failed(asset_ptr, error_message);
    }

    if (on_failure_callback != nullptr) {
      on_failure_callback(asset_ptr, error_message);
    }
  }

  namespace detail {

    void load_texture(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void load_model_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null");
      OTHER_ASSERT(on_success != nullptr, "on_success callback is null");
      OTHER_ASSERT(on_failure != nullptr, "on_failure callback is null");
      OTHER_ASSERT(std::filesystem::exists(asset_ptr->absolute_path), "Model source file does not exist: {}", asset_ptr->load_path.string());

      CORE_LOG_DEBUG("Loading model source from file: {}", asset_ptr->load_path.string());
      model_builder builder = model_importer::load_model_data(asset_ptr->absolute_path);
      CORE_LOG_DEBUG("Model source data loaded from file: {}", asset_ptr->load_path.string());

      model_source_pipeline* pl = reinterpret_cast<model_source_pipeline*>(pipeline);
      OTHER_ASSERT(pl != nullptr, "Pipeline is null!");

      if (builder.vertices.empty() || builder.indices.empty() || builder.submeshes.empty() || builder.nodes.empty()) {
        (pl->*on_failure)(std::format("Failed to load model source: {} (data invalid)", asset_ptr->load_path.string()));
      } else {
        (pl->*on_success)();
        {
          std::lock_guard lck{ pl->mtx };
          pl->builder = std::move(builder);
        }
      }
    }

    void load_model(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void load_script_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void load_audio(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void empty_loader(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      (reinterpret_cast<asset_pipeline*>(pipeline)->*on_success)();
    }

    void unload_texture(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void unload_model_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      CORE_LOG_DEBUG("Unloading model source (ID: {})", asset_ptr->id);

      /// nothing to do here for now since renderer_backend handles it,
      ///  later we will want to check if there is anything that needs to be written to disk, etc.
      /// most of the work has to happen in the renderer_backend on the rendering thread
      model_source_pipeline* pl = reinterpret_cast<model_source_pipeline*>(pipeline);
      OTHER_ASSERT(pl != nullptr, "Pipeline is null!");
      (pl->*on_success)();
    }

    void unload_model(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void unload_script_source(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void unload_audio(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

    void empty_unloader(asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
    }

  }  // namespace detail
}  // namespace other