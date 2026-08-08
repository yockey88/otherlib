/**
 * \file asset/asset_pipeline.cpp
 **/
#include "asset/asset_pipeline.hpp"

#include <filesystem>

#include "core/job_system.hpp"
#include "core/profiler.hpp"
#include "file/filesystem.hpp"
#include "file/path_helpers.hpp"
#include "serialization/scene_serializer.hpp"

#include "gpu_resource/material.hpp"
#include "gpu_resource/texture_importer.hpp"
#include "model/model_source.hpp"
#include "renderer/pipeline_definition.hpp"
#include "script/scripting_environment.hpp"

#include "scene/scene.hpp"

#include "tools/project_tool.hpp"

#include "serialization/animation_serializer.hpp"

#include "audio/audio_environment.hpp"
#include "audio/audio_import.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
#include "asset/pipelines/animation_pipeline.hpp"
#include "asset/pipelines/asset_declaration_pipeline.hpp"
#include "asset/pipelines/audio_pipeline.hpp"
#include "asset/pipelines/material_pipeline.hpp"
#include "asset/pipelines/model_source_pipeline.hpp"
#include "asset/pipelines/rendering_pipeline_pipeline.hpp"
#include "asset/pipelines/scene_pipeline.hpp"
#include "asset/pipelines/script_file_pipeline.hpp"
#include "asset/pipelines/script_pipeline.hpp"
#include "asset/pipelines/script_project_pipeline.hpp"
#include "asset/pipelines/script_source_pipeline.hpp"
#include "asset/pipelines/stub_pipeline.hpp"
#include "asset/pipelines/texture_pipeline.hpp"

namespace other {

  namespace detail {

    task load_texture(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_audio(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_stub_asset(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_animation(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_material(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task empty_loader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

    task unload_texture(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_audio(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_stub_asset(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_animation(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_material(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task empty_unloader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

  }  // namespace detail

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::loaders = {
    detail::load_texture,
    detail::load_model_source,
    detail::load_animation,
    detail::load_script_project,
    detail::load_script_source,
    detail::load_script_file,
    detail::load_script,
    detail::load_audio,
    detail::load_scene,
    detail::load_stub_asset,  // INPUT_MAP: asset model undefined; runtime input_map exists
    detail::load_rendering_pipeline,
    detail::load_asset_declaration,
    detail::load_material,
    detail::empty_loader,
  };

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::unloaders = {
    detail::unload_texture,
    detail::unload_model_source,
    detail::unload_animation,
    detail::unload_script_project,
    detail::unload_script_source,
    detail::unload_script_file,
    detail::unload_script,
    detail::unload_audio,
    detail::unload_scene,
    detail::unload_stub_asset,
    detail::unload_rendering_pipeline,
    detail::unload_asset_declaration,
    detail::unload_material,
    detail::empty_unloader,
  };

  bool asset_pipeline::is_extension_supported(const std::string_view extension) {
    return std::ranges::find_if(kAssetExtensions, [extension](const auto& ext) { return ext.extension == extension; }) != kAssetExtensions.end();
  }

  scope<asset_pipeline> asset_pipeline::get_asset_pipeline(event_system* events, asset_handler* handler, asset::type type) {
    switch (type) {
      case asset::TEXTURE: return make_scope<texture_pipeline>(events, handler);
      case asset::ANIMATION: return make_scope<animation_pipeline>(events, handler);
      case asset::AUDIO: return make_scope<audio_pipeline>(events, handler);
      case asset::INPUT_MAP: return make_scope<stub_pipeline>(events, handler);
      case asset::MODEL_SOURCE: return make_scope<model_source_pipeline>(events, handler);
      case asset::SCRIPT_PROJECT: return make_scope<script_project_pipeline>(events, handler);
      case asset::SCRIPT_SOURCE: return make_scope<script_source_pipeline>(events, handler);
      case asset::SCRIPT_FILE: return make_scope<script_file_pipeline>(events, handler);
      case asset::SCRIPT: return make_scope<script_pipeline>(events, handler);
      case asset::SCENE: return make_scope<scene_pipeline>(events, handler, nullptr);
      case asset::RENDERING_PIPELINE: return make_scope<rendering_pipeline_pipeline>(events, handler);
      case asset::ASSET_DECLARATION: return make_scope<asset_declaration_pipeline>(events, handler);
      case asset::MATERIAL: return make_scope<material_pipeline>(events, handler);
      default:
        OTHER_ASSERT(false, "No asset pipeline for asset type {}", type);
    }
  }

  scope<asset_pipeline> asset_pipeline::get_model_source_pipeline(event_system* events, asset_handler* handler, const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices) {
    PROFILE_SECTION("asset_pipeline::get_model_source_pipeline");
    CORE_LOG_DEBUG("Building model source pipeline for model '{}', vertex count {}, index count {}", name, vertices.size(), indices.size());
    scope<model_source_pipeline> pl = make_scope<model_source_pipeline>(events, handler);
    pl->data = build(name, vertices, indices);
    return pl;
  }

  scope<asset_pipeline> asset_pipeline::get_scene_pipeline(event_system* events, asset_handler* handler, scene* scene_ptr) {
    return make_scope<scene_pipeline>(events, handler, scene_ptr);
  }

  scope<asset_pipeline> asset_pipeline::get_rendering_pipeline_pipeline(event_system* events, asset_handler* handler, const pipeline_definition& definition) {
    return make_scope<rendering_pipeline_pipeline>(events, handler, definition);
  }

  void asset_pipeline::set_asset_data(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in set_asset_data");
    this->asset_ptr = asset_ptr;
    pipeline_state.success = true;
  }

  void asset_pipeline::start_load(job_system& jobs, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in start_load");
    OTHER_ASSERT(on_success != nullptr, "on_success callback is null in start_load");
    OTHER_ASSERT(on_failure != nullptr, "on_failure callback is null in start_load");
    CORE_LOG_DEBUG("Starting load pipeline for asset ID: {}", asset_ptr->id);

    if (pipeline_state.loading) {
      CORE_LOG_WARN("Pipeline is already loading for asset ID: {}", asset_ptr->id);
      return;
    }

    /// \todo look up custom asset loader if need be (will have to reference symbol in plugin)

    pipeline_state.loading = true;
    error_message = "";
    start_load_operation(
      jobs, asset_ptr, on_success, on_failure,
      loading_table::loaders[asset_ptr->asset_type]);
  }

  void asset_pipeline::start_unload(job_system& jobs, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
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

    /// \todo look up custom asset unloader if need be (will have to reference symbol in plugin)

    pipeline_state.unloading = true;
    error_message = "";
    start_load_operation(
      jobs, asset_ptr, on_success, on_failure,
      loading_table::unloaders[asset_ptr->asset_type]);
  }

  void asset_pipeline::poll() {
    if (!pipeline_state.loading && !pipeline_state.unloading) {
      return;
    }

    if (pipeline_state.success) {
      pipeline_complete(asset_ptr);
    } else if (pipeline_state.failure) {
      pipeline_failed(asset_ptr, error_message);
    }

    if (pipeline_state.success || pipeline_state.failure) {
      reset();
    }
  }

  void asset_pipeline::start_load_operation(job_system& jobs, asset* asset_ptr, on_asset_loaded on_success, on_asset_load_failed on_failure, loading_table::loader_fn_t function) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null");
    OTHER_ASSERT(function != nullptr, "Loader function pointer is null");
    OTHER_ASSERT(handler != nullptr, "Asset handler pointer is null");
    PROFILE_SECTION("asset_pipeline::start_load_operation");

    on_success_callback = on_success;
    on_failure_callback = on_failure;
    this->asset_ptr = asset_ptr;

    CORE_LOG_TRACE("Posting asset load operation for asset: [{}]", asset_ptr->id);
    jobs.post_coroutine(function(handler, asset_ptr, &asset_pipeline::pipeline_finished, &asset_pipeline::pipeline_failed, this));
  }

  void asset_pipeline::pipeline_finished() {
    pipeline_state.success = true;
  }

  void asset_pipeline::pipeline_failed(const std::string_view err_msg) {
    error_message = std::string(err_msg);
    pipeline_state.failure = true;
  }

  void asset_pipeline::reset() {
    pipeline_state.success = false;
    pipeline_state.failure = false;

    pipeline_state.loading = false;
    pipeline_state.unloading = false;
  }

  void asset_pipeline::pipeline_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in pipeline_complete");
    OTHER_ASSERT(pipeline_state.loading || pipeline_state.unloading, "Pipeline is not in loading or unloading state in pipeline_complete");
    PROFILE_SECTION("asset_pipeline::pipeline_complete");
    CORE_LOG_TRACE("Pipeline complete for asset ID: {}", asset_ptr->id);

    std::string event_name = "";
    if (pipeline_state.loading) {
      event_name = "asset-loaded";
    } else if (pipeline_state.unloading) {
      event_name = "asset-unloaded";
    }

    get_events().trigger_event(get_asset_event_name(asset_ptr->asset_type, event_name), asset_ptr->id);

    /// \todo remove this
    if (on_success_callback != nullptr) {
      on_success_callback(asset_ptr);
    }
  }

  void asset_pipeline::pipeline_failed(asset* asset_ptr, const std::string_view error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in pipeline_failed");
    OTHER_ASSERT(pipeline_state.loading || pipeline_state.unloading, "Pipeline is not in loading or unloading state in pipeline_failed");
    PROFILE_SECTION("asset_pipeline::pipeline_failed");

    std::string event_name = "";
    if (pipeline_state.loading) {
      event_name = "asset-load-failed";
    } else if (pipeline_state.unloading) {
      event_name = "asset-unload-failed";
    }

    /// \todo remove this
    if (on_failure_callback != nullptr) {
      on_failure_callback(asset_ptr, error_message);
    }
    get_events().trigger_event(get_asset_event_name(asset_ptr->asset_type, event_name), asset_ptr->id);
  }

  namespace detail {

    void verify_parameters(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      OTHER_ASSERT(handler != nullptr, "Asset handler pointer is null in pipeline function");
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in pipeline function");
      OTHER_ASSERT(on_success != nullptr, "on_success callback is null in pipeline function");
      OTHER_ASSERT(on_failure != nullptr, "on_failure callback is null in pipeline function");
      OTHER_ASSERT(pipeline != nullptr, "Pipeline pointer is null in pipeline function");
    }

    template <typename T, typename Fn, typename... Args>
    void call_pipeline_fn(void* pipeline, Fn function, Args&&... args) {
      OTHER_ASSERT(pipeline != nullptr, "Pipeline pointer is null in call_pipeline_fn");
      (reinterpret_cast<T*>(pipeline)->*function)(std::forward<Args>(args)...);
    }

    task load_texture(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      const filepath source_path = asset_ptr->absolute_path;
      if (!std::filesystem::exists(source_path)) {
        call_pipeline_fn<texture_pipeline>(pipeline, on_failure, std::format("Texture file does not exist: {}", source_path.string()));
        co_return;
      }
      CORE_LOG_DEBUG("Loading texture from file: {}", source_path.string());

      texture_importer::texture_data data;
      ref<job> decode_job = handler->get_job_system().submit(
        {
          .name = std::format("Decode Texture Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          .thread_affinity = job::affinity::WORKER_THREAD,
        },
        [&data, source_path]() {
          /// local to this coroutine; the upload job depends on this one, so no concurrent access
          data = texture_importer::load_texture_data(source_path);
        });

      ref<job> upload_job = handler->get_job_system().submit(
        {
          .name = std::format("Upload Texture Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// gpu upload must happen on the thread owning the gl context
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [asset_ptr, d = &data]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in upload texture job");
          if (!d->valid()) {
            throw std::runtime_error(std::format("Failed to decode texture asset: {}", asset_ptr->load_path.string()));
          }

          const std::string name = asset_ptr->load_path.filename().stem().string();
          resource_handle handle = texture_importer::upload_texture_data(name, *d);
          if (handle.id == 0) {
            throw std::runtime_error(std::format("Failed to upload texture asset: {}", asset_ptr->load_path.string()));
          }

          subsystem<renderer_backend>::get()->add_texture(asset_ptr->path_hash, handle);
          CORE_LOG_DEBUG("Texture loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        std::array{ decode_job->id });

      do {
        co_await task::yield();
      } while (!upload_job->done());

      if (upload_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<texture_pipeline>(pipeline, on_success);
      } else {
        call_pipeline_fn<texture_pipeline>(pipeline, on_failure, std::format("Failed to load texture asset: {}", source_path.string()));
      }
    }

    task load_stub_asset(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      CORE_LOG_WARN("No {} backend yet; asset '{}' is tracked but carries no runtime data", asset_ptr->asset_type, asset_ptr->load_path.string());
      co_await task::yield();
      call_pipeline_fn<stub_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_audio(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      const filepath source_path = asset_ptr->absolute_path;
      if (!std::filesystem::exists(source_path)) {
        call_pipeline_fn<audio_pipeline>(pipeline, on_failure, std::format("Audio file does not exist: {}", source_path.string()));
        co_return;
      }
      CORE_LOG_DEBUG("Loading audio from file: {}", source_path.string());

      audio_import_result imported;
      ref<job> decode_job = handler->get_job_system().submit(
        {
          .name = std::format("Decode Audio Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// standalone decoder instance, never the engine — safe off the main thread
          .thread_affinity = job::affinity::WORKER_THREAD,
        },
        [&imported, source_path]() {
          PROFILE_SECTION("load_audio--decode");
          /// local to this coroutine; the register job depends on this one, so no concurrent access
          imported = import_audio(source_path);
        });

      ref<job> register_job = handler->get_job_system().submit(
        {
          .name = std::format("Register Audio Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// the clip registry is main-thread-owned state
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [asset_ptr, r = &imported]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register audio job");
          if (!r->success()) {
            throw std::runtime_error(r->error);
          }
          for (const std::string& warning : r->warnings) {
            CORE_LOG_WARN("audio '{}': {}", asset_ptr->load_path.string(), warning);
          }

          if (subsystem<audio_environment>::inert) {
            /// minimal profiles: tracked-no-payload, the stub's contract preserved
            CORE_LOG_WARN("Audio environment inert; asset '{}' is tracked but carries no runtime data", asset_ptr->load_path.string());
            return;
          }
          subsystem<audio_environment>::get()->add_clip(asset_ptr->path_hash, std::move(*r->clip));
          CORE_LOG_DEBUG("Audio clip loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        std::array{ decode_job->id });

      do {
        co_await task::yield();
      } while (!register_job->done());

      if (register_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<audio_pipeline>(pipeline, on_success);
      } else if (!imported.error.empty()) {
        call_pipeline_fn<audio_pipeline>(pipeline, on_failure, imported.error);
      } else {
        call_pipeline_fn<audio_pipeline>(pipeline, on_failure, std::format("Failed to load audio asset: {}", source_path.string()));
      }
    }

    /// standalone .oanim clips; embedded clips ride their model_source instead
    task load_animation(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      const filepath source_path = asset_ptr->absolute_path;
      if (!std::filesystem::exists(source_path)) {
        call_pipeline_fn<animation_pipeline>(pipeline, on_failure, std::format("Animation clip file does not exist: {}", source_path.string()));
        co_return;
      }
      CORE_LOG_DEBUG("Loading animation clip from file: {}", source_path.string());

      serialization::clip_parse_result parsed;
      ref<job> parse_job = handler->get_job_system().submit(
        {
          .name = std::format("Parse Animation Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// pure cpu binary parse, keep it off the main thread
          .thread_affinity = job::affinity::WORKER_THREAD,
        },
        [&parsed, source_path]() {
          /// local to this coroutine; the register job depends on this one, so no concurrent access
          parsed = serialization::load_animation_clip(source_path);
        });

      ref<job> register_job = handler->get_job_system().submit(
        {
          .name = std::format("Register Animation Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// the clip registry is main-thread-owned state
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [asset_ptr, p = &parsed]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register animation job");
          if (!p->success()) {
            throw std::runtime_error(p->error);
          }

          subsystem<renderer_backend>::get()->add_animation(asset_ptr->path_hash, std::move(*p->clip));
          CORE_LOG_DEBUG("Animation clip loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        std::array{ parse_job->id });

      do {
        co_await task::yield();
      } while (!register_job->done());

      if (register_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<animation_pipeline>(pipeline, on_success);
      } else if (!parsed.error.empty()) {
        call_pipeline_fn<animation_pipeline>(pipeline, on_failure, parsed.error);
      } else {
        call_pipeline_fn<animation_pipeline>(pipeline, on_failure, std::format("Failed to load animation asset: {}", source_path.string()));
      }
    }

    /// promotes the importer's value-sets to plain materials owned by the model_source (derived
    ///  data, not assets/files); gltf textures are resolver-declared (dedupes), legacy formats get a lazy kick-off here
    static ostd::vector<material> build_imported_materials(const model_data& data, asset* asset_ptr, asset_handler* handler) {
      PROFILE_SECTION("build_imported_materials");
      ostd::vector<material> out;
      if (data.materials.empty()) {
        return out;
      }

      auto* fs = subsystem<file_system>::get();
      out.reserve(data.materials.size());
      for (const imported_material& imp : data.materials) {
        material& mat = out.emplace_back();
        mat.name = imp.name;

        const auto set_param = [&mat](std::string_view name, material_value value) {
          const natural_t hash = FNV(name);
          mat.params[hash] = value;
          mat.param_names[hash] = std::string{ name };
        };
        set_param("base_color", material_value::from(imp.base_color));
        set_param("emissive_color", material_value::from(imp.emissive_color));
        set_param("roughness", material_value::from(imp.roughness));
        set_param("metalness", material_value::from(imp.metalness));

        const auto set_slot = [&](std::string_view slot_name, const std::string& authored) {
          if (authored.empty()) {
            return;
          }
          if (authored.starts_with("embedded:")) {
            CORE_LOG_WARN("model '{}': embedded texture '{}' is not supported yet; slot binds a 1x1 fallback", asset_ptr->load_path.string(), authored);
            return;
          }
          filepath abs = resolve_relative(asset_ptr->absolute_path, authored);
          if (!std::filesystem::exists(abs)) {
            CORE_LOG_WARN("model '{}': texture '{}' does not exist; slot binds a 1x1 fallback", asset_ptr->load_path.string(), abs.string());
            return;
          }
          if (fs != nullptr && fs->deep_search_for_mount(abs).is_valid()) {
            abs = absolute_of(virtualize(abs));  /// canonical form, byte-identical with resolver dispatch
          }
          const natural_t texture_id = handler->load_asset(abs);
          if (texture_id == 0) {
            CORE_LOG_WARN("model '{}': texture '{}' could not begin loading; slot binds a 1x1 fallback", asset_ptr->load_path.string(), abs.string());
            return;
          }
          const natural_t hash = FNV(slot_name);
          mat.texture_paths[hash] = authored;
          mat.texture_hashes[hash] = handler->get_asset_hash(texture_id);
        };
        set_slot("base_color", imp.base_color_texture);
        set_slot("normal", imp.normal_texture);
        set_slot("metallic_roughness", imp.metallic_roughness_texture);
        set_slot("emissive", imp.emissive_texture);
      }
      return out;
    }

    task load_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      bool load_model = true;
      filepath source_path = {};
      if (asset_ptr->path_hash == 0) {
        load_model = false;
        source_path = asset_ptr->virtual_path;
      } else {
        source_path = asset_ptr->absolute_path;
      }

      if (load_model && !std::filesystem::exists(source_path)) {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_failure, std::format("Model source file does not exist: {}", source_path.string()));
        co_return;
      }

      model_import_result result;
      ostd::vector<natural_t> dependencies = {};

      ref<job> import_job = nullptr;
      if (load_model) {
        CORE_LOG_DEBUG("Loading model source from file: {}", source_path.string());

        import_job = handler->get_job_system().submit(
          {
            .name = std::format("Import Model Source Asset {}", asset_ptr->id),
            .priority = job::priority::LOW,
            /// pure cpu import that can take arbitrarily long, so keep it off the main thread
            .thread_affinity = job::affinity::WORKER_THREAD,
          },
          [&result, source_path]() {
            /// local to this coroutine; the store job depends on this one, so no concurrent access
            result = import(source_path);
          });

        dependencies.push_back(import_job->id);
      } else {
        CORE_LOG_DEBUG(" - finalizing model upload for model {}", asset_ptr->virtual_path.string());
        asset_ptr->path_hash = FNV(asset_ptr->virtual_path.string());
        result.data = std::move(reinterpret_cast<model_source_pipeline*>(pipeline)->data);
      }

      auto store_job = handler->get_job_system().submit(
        {
          .name = std::format("Finalize Load Model Source Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// runs on main thread because the model_source constructor uploads to the gpu
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [handler, asset_ptr, r = &result]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in finalize load model source job");
          if (!r->data.has_value() || !r->data->valid()) {
            throw std::runtime_error(std::format("Failed to load model source asset {}: {}", asset_ptr->load_path.string(),
                                                 r->error.empty() ? "model data invalid" : r->error));
          }

          ref<model_source> src = make_ref<model_source>(std::move(*r->data));
          src->set_imported_materials(build_imported_materials(src->source_data(), asset_ptr, handler));

          renderer_backend* renderer = subsystem<renderer_backend>::get();
          renderer->upload_model(*src);
          renderer->add_model_source(asset_ptr->path_hash, src);
          CORE_LOG_DEBUG("Model source loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        dependencies);

      /// waiting on this will also wait on the import job
      do {
        co_await task::yield();
      } while (!store_job->done());
      OTHER_ASSERT(store_job->done(), "Store job for model source asset in invalid state. Status: {}", store_job->get_status());

      if (store_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_success);
      } else if (!result.error.empty()) {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_failure, std::format("Failed to load model source asset {}: {}", source_path.string(), result.error));
      } else {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_failure, std::format("Failed to finalize model source asset: {}", source_path.string()));
      }
    }

    task load_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      filepath project_path = asset_ptr->absolute_path;
      auto& jobs = handler->get_job_system();

      ref<project_tool> build_tool = make_ref<project_tool>();
      ref<job> build_project_job = jobs.submit(
        {
          .name = std::format("Build .NET project '{}'", project_path.string()),
          .priority = job::priority::LOW,
          .thread_affinity = job::affinity::WORKER_THREAD,
        },
        [t = build_tool, path = project_path]() mutable {
          OTHER_ASSERT(t != nullptr, "Failed to create project tool for building .NET project.");
          /// csproj generation is a project-creation concern; the resolver refuses a
          //  missing root long before this pipeline runs
          OTHER_ASSERT(std::filesystem::exists(path), "csproj '{}' vanished between resolve and build", path.string());
          PROFILE_SECTION("load_script_project--dotnet-build");
          t->start_project_build(path);

          do {
            std::this_thread::yield();
          } while (t->project_build_in_progress());

          int32_t result = t->get_build_result();
          t->cleanup_build();
          t = nullptr;

          if (result == 0) {
            CORE_LOG_DEBUG("Successfully built .NET project '{}'", path.string());
          } else {
            throw std::runtime_error(std::format("Failed to build .NET project '{}'. Build result code: {}", path.string(), result));
          }
        });
      OTHER_ASSERT(build_project_job != nullptr, "Failed to create job for building .NET project.");

      do {
        co_await task::yield();
      } while (!build_project_job->done());

      if (build_project_job->get_status() != job::status::COMPLETED) {
        call_pipeline_fn<script_project_pipeline>(pipeline, on_failure, std::format("Loading built assembly for .NET project '{}' was cancelled.", project_path.string()));
        co_return;
      }
      CORE_LOG_DEBUG("Successfully loaded built assembly for .NET project '{}'", project_path.string());

      build_tool = nullptr;
      call_pipeline_fn<script_project_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      filepath script_path = asset_ptr->absolute_path;
      OTHER_ASSERT(std::filesystem::exists(script_path), "Script file does not exist: {}", script_path.string());
      if (script_path.extension() == ".dll" || script_path.extension() == ".so" /* || script_path.extension() == ".dylib" ? */) {
        CORE_LOG_DEBUG("Loading C# script from file: {}", script_path.string());

        auto* env = subsystem<scripting_environment>::get();
        OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not available.");
        // just checking it was successful, no need to do anything with the ref here
        ref<assembly> asm_ref = env->load_dotnet_module(script_path.string());
        if (!asm_ref) {
          call_pipeline_fn<script_pipeline>(pipeline, on_failure, std::format("Failed to load assembly for script asset: {}", script_path.string()));
          co_return;
        }

        CORE_LOG_DEBUG("Successfully loaded assembly for script asset: {}", script_path.string());
      } else if (script_path.extension() == ".lua") {
        CORE_LOG_DEBUG("Loading Lua script from file: {}", script_path.string());
        CORE_LOG_WARN("Lua scripting is not yet implemented, treating Lua script asset as empty for now");
      } else if (script_path.extension() == ".py") {
        CORE_LOG_DEBUG("Loading Python script from file: {}", script_path.string());
        CORE_LOG_WARN("Python scripting is not yet implemented, treating Python script asset as empty for now");
      } else {
        OTHER_ASSERT(false, "Unsupported script type for file: {}", script_path.string());
      }

      call_pipeline_fn<script_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      filepath script_path = asset_ptr->absolute_path;
      OTHER_ASSERT(std::filesystem::exists(script_path), "Script file does not exist: {}", script_path.string());
      CORE_LOG_DEBUG("Loading script file from file: {}", script_path.string());

      /// \todo implement loading script files (lua, py, etc)

      call_pipeline_fn<script_file_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      /// \todo implement this
      co_await task::yield();
      call_pipeline_fn<script_source_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      /// live scenes enter via add_scene_asset (path_hash == 0, scene attached); resolver-dispatched
      /// docs (path_hash != 0, no scene) only parse to validate/track — instantiation re-parses via the scene graph
      if (asset_ptr->path_hash != 0) {
        const std::string standalone_extension = asset_ptr->load_path.extension().string();
        if (!serialization::is_scene_file_extension(standalone_extension)) {
          call_pipeline_fn<scene_pipeline>(pipeline, on_failure, std::format("'{}' is not a scene document ({}/{} expected)", asset_ptr->load_path.string(), serialization::kSceneTomlExtension, serialization::kSceneBinaryExtension));
          co_return;
        }

        serialization::scene_parse_result parsed = serialization::load_scene_document(asset_ptr->absolute_path);
        if (!parsed.success()) {
          call_pipeline_fn<scene_pipeline>(pipeline, on_failure, std::format("failed to parse scene document '{}': {}", asset_ptr->load_path.string(), parsed.error));
          co_return;
        }
        for (const std::string& warning : parsed.warnings) {
          CORE_LOG_WARN("scene document '{}': {}", asset_ptr->load_path.string(), warning);
        }

        co_await task::yield();
        call_pipeline_fn<scene_pipeline>(pipeline, on_success);
        co_return;
      }

      asset_ptr->path_hash = FNV(asset_ptr->virtual_path.string());
      scene* scene_ptr = reinterpret_cast<scene_pipeline*>(pipeline)->scene_ptr;
      OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene pipeline.");

      /// file-backed scenes parse their document here (pure, safe off the main thread);
      /// activation instantiates it on the main thread (scene::instantiate_pending_document)
      const std::string extension = asset_ptr->load_path.extension().string();
      if (serialization::is_scene_file_extension(extension)) {
        serialization::scene_parse_result parsed = serialization::load_scene_document(asset_ptr->absolute_path);
        if (!parsed.success()) {
          call_pipeline_fn<scene_pipeline>(pipeline, on_failure, std::format("failed to parse scene document '{}': {}", asset_ptr->load_path.string(), parsed.error));
          co_return;
        }
        for (const std::string& warning : parsed.warnings) {
          CORE_LOG_WARN("scene document '{}': {}", asset_ptr->load_path.string(), warning);
        }
        scene_ptr->source_path = asset_ptr->absolute_path;
        scene_ptr->set_pending_document(std::move(*parsed.document));
      } else if (!asset_ptr->load_path.empty()) {
        call_pipeline_fn<scene_pipeline>(pipeline, on_failure, std::format("'{}' is not a scene document ({}/{} expected)", asset_ptr->load_path.string(), serialization::kSceneTomlExtension, serialization::kSceneBinaryExtension));
        co_return;
      }

      scene_ptr->asset_id = asset_ptr->id;
      co_await task::yield();

      CORE_LOG_DEBUG("Scene [{}: {}] loaded successfully with asset ID: {}.", scene_ptr->name, scene_ptr->id, scene_ptr->asset_id);
      call_pipeline_fn<scene_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      co_await task::yield();

      if (asset_ptr->path_hash == 0) {
        /// builtin rendering pipeline nothing to do
        call_pipeline_fn<rendering_pipeline_pipeline>(pipeline, on_success);
        co_return;
      }

      rendering_pipeline_pipeline* pipeline_ptr = reinterpret_cast<rendering_pipeline_pipeline*>(pipeline);

      auto definition = read_pipeline_definition_from_file(asset_ptr->load_path);
      if (definition.name.empty()) {
        call_pipeline_fn<rendering_pipeline_pipeline>(pipeline, on_failure, "Rendering pipeline definition is invalid: name is empty");
        co_return;
      }

      pipeline_ptr->definition = definition;
      call_pipeline_fn<rendering_pipeline_pipeline>(pipeline, on_success);
    }

    task load_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      OTHER_ASSERT(std::filesystem::exists(asset_ptr->load_path), "Asset declaration does not exist! {}", asset_ptr->load_path.string());

      toml::table table;
      try {
        std::string contents;
        {
          std::stringstream ss;
          std::ifstream file(asset_ptr->load_path);
          if (!file.is_open()) {
            call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_failure, std::format("Asset declarataion loading error: {} could not be found", asset_ptr->load_path.string()));
            co_return;
          }

          ss << file.rdbuf();
          contents = ss.str();
        }
        table = toml::parse(contents);
      } catch (const std::exception& e) {
        call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_failure, std::format("Asset declarataion loading error: {}", e.what()));
        co_return;
      }

      if (!table.contains("asset-type")) {
        call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_failure, "Asset declaration is missing asset type!");
        co_return;
      }

      auto& type_node = table.at("asset-type");
      if (!type_node.is_string()) {
        call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_failure, std::format("Asset type must be of stype [string]. it is of type: {}", type_node.type()));
        co_return;
      }

      std::string type = type_node.as_string()->get();
      CORE_LOG_DEBUG("Loading Asset Declaration of type: {}", type);

      call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_material(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      const filepath source_path = asset_ptr->absolute_path;
      if (!std::filesystem::exists(source_path)) {
        call_pipeline_fn<material_pipeline>(pipeline, on_failure, std::format("Material file does not exist: {}", source_path.string()));
        co_return;
      }
      CORE_LOG_DEBUG("Loading material from file: {}", source_path.string());

      material_parse_result parsed;
      ref<job> parse_job = handler->get_job_system().submit(
        {
          .name = std::format("Parse Material Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// pure cpu toml parse, keep it off the main thread
          .thread_affinity = job::affinity::WORKER_THREAD,
        },
        [&parsed, source_path]() {
          PROFILE_SECTION("load_material--parse");
          /// local to this coroutine; the register job depends on this one, so no concurrent access
          parsed = parse_material_toml(source_path);
        });

      ref<job> register_job = handler->get_job_system().submit(
        {
          .name = std::format("Register Material Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// registration + texture kick-offs touch main-thread-owned state
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [handler, asset_ptr, p = &parsed]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register material job");
          if (!p->success()) {
            throw std::runtime_error(p->error);
          }
          for (const std::string& warning : p->warnings) {
            CORE_LOG_WARN("{}", warning);
          }

          auto* fs = subsystem<file_system>::get();
          OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in register material job");

          material mat = std::move(*p->mat);
          /// slot paths are relative to the .omat; the texture hash is decided by the asset handler at
          //  load time — never predict it, load (idempotent) and read it back; this is the lazy kick-off for legacy/standalone materials
          for (const auto& [slot, rel] : mat.texture_paths) {
            if (rel.empty()) {
              continue;
            }
            filepath abs = resolve_relative(asset_ptr->absolute_path, rel);
            if (!std::filesystem::exists(abs)) {
              CORE_LOG_WARN("material '{}': texture '{}' does not exist; slot binds a 1x1 fallback", asset_ptr->load_path.string(), abs.string());
              continue;
            }
            if (fs->deep_search_for_mount(abs).is_valid()) {
              abs = absolute_of(virtualize(abs));  /// canonical form, byte-identical with resolver dispatch
            }
            const natural_t texture_id = handler->load_asset(abs);
            if (texture_id == 0) {
              CORE_LOG_WARN("material '{}': texture '{}' could not begin loading; slot binds a 1x1 fallback", asset_ptr->load_path.string(), abs.string());
              continue;
            }
            mat.texture_hashes[slot] = handler->get_asset_hash(texture_id);
          }

          subsystem<renderer_backend>::get()->add_material(asset_ptr->path_hash, std::move(mat));
          CORE_LOG_DEBUG("Material loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        std::array{ parse_job->id });

      do {
        co_await task::yield();
      } while (!register_job->done());

      if (register_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<material_pipeline>(pipeline, on_success);
      } else if (!parsed.error.empty()) {
        call_pipeline_fn<material_pipeline>(pipeline, on_failure, parsed.error);
      } else {
        call_pipeline_fn<material_pipeline>(pipeline, on_failure, std::format("Failed to load material asset: {}", source_path.string()));
      }
    }

    task empty_loader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      OTHER_ASSERT(false, "No loader implemented for asset type {} in empty_loader", asset_ptr->asset_type);
      co_return;
    }

    task unload_texture(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading texture (ID: {})", asset_ptr->id);

      auto* renderer = subsystem<renderer_backend>::get();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend subsystem is not available in unload_texture");

      renderer->remove_texture(asset_ptr->path_hash);
      call_pipeline_fn<texture_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_stub_asset(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading {} stub asset (ID: {})", asset_ptr->asset_type, asset_ptr->id);
      call_pipeline_fn<stub_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_audio(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading audio clip (ID: {})", asset_ptr->id);

      if (!subsystem<audio_environment>::inert) {
        auto* env = subsystem<audio_environment>::get();
        OTHER_ASSERT(env != nullptr, "Audio environment subsystem is not available in unload_audio");
        /// voices reading this clip's PCM die on the main thread before the buffer
        ///  is freed — the remove_clip no-live-voices contract, satisfied by order
        env->stop_voices_on(asset_ptr->path_hash);
        env->remove_clip(asset_ptr->path_hash);
      }
      call_pipeline_fn<audio_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_animation(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading animation clip (ID: {})", asset_ptr->id);

      auto* renderer = subsystem<renderer_backend>::get();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend subsystem is not available in unload_animation");

      renderer->remove_animation(asset_ptr->path_hash);
      call_pipeline_fn<animation_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading model source (ID: {})", asset_ptr->id);

      auto* renderer = subsystem<renderer_backend>::get();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend subsystem is not available in unload_scene");

      renderer->remove_model_source(asset_ptr->path_hash);
      call_pipeline_fn<model_source_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading script project (ID: {})", asset_ptr->id);
      call_pipeline_fn<script_project_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading script source (ID: {})", asset_ptr->id);
      call_pipeline_fn<script_source_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading script file (ID: {})", asset_ptr->id);
      call_pipeline_fn<script_file_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading script (ID: {})", asset_ptr->id);
      call_pipeline_fn<script_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading scene (ID: {})", asset_ptr->id);
      call_pipeline_fn<scene_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading rendering pipeline (ID: {})", asset_ptr->id);
      call_pipeline_fn<rendering_pipeline_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      call_pipeline_fn<asset_declaration_pipeline>(pipeline, on_success);
      co_return;
    }

    task unload_material(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      CORE_LOG_DEBUG("Unloading material (ID: {})", asset_ptr->id);

      auto* renderer = subsystem<renderer_backend>::get();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend subsystem is not available in unload_material");

      renderer->remove_material(asset_ptr->path_hash);
      call_pipeline_fn<material_pipeline>(pipeline, on_success);
      co_return;
    }

    task empty_unloader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      OTHER_ASSERT(false, "No loader implemented for asset type {} in empty_loader", asset_ptr->asset_type);
      co_return;
    }

  }  // namespace detail
}  // namespace other