/**
 * \file asset/asset_pipeline.cpp
 **/
#include "asset/asset_pipeline.hpp"

#include "core/job_system.hpp"

#include "model/model_source.hpp"
#include "script/scripting_environment.hpp"

#include "scene/scene.hpp"

#include "tools/project_tool.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"
#include "asset/pipelines/asset_declaration_pipeline.hpp"
#include "asset/pipelines/model_source_pipeline.hpp"
#include "asset/pipelines/rendering_pipeline_pipeline.hpp"
#include "asset/pipelines/scene_pipeline.hpp"
#include "asset/pipelines/script_file_pipeline.hpp"
#include "asset/pipelines/script_pipeline.hpp"
#include "asset/pipelines/script_project_pipeline.hpp"
#include "asset/pipelines/script_source_pipeline.hpp"

namespace other {

  namespace detail {

    task load_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task load_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task empty_loader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

    task unload_model_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_source(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script_file(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_script(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_scene(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task unload_asset_declaration(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);
    task empty_unloader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline);

  }  // namespace detail

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::loaders = {
    detail::empty_loader,  // detail::load_texture,
    detail::load_model_source,
    detail::empty_loader,  // detail::load_model,
    detail::empty_loader,  // detail::load_animation,
    detail::load_script_project,
    detail::load_script_source,
    detail::load_script_file,
    detail::load_script,
    detail::empty_loader,  // detail::load_audio,
    detail::load_scene,
    detail::empty_loader,  // detail::load_input_map,
    detail::load_rendering_pipeline,
    detail::load_asset_declaration,
    detail::empty_loader,
  };

  std::array<asset_pipeline::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_pipeline::loading_table::unloaders = {
    detail::empty_unloader,  // detail::unload_texture,
    detail::unload_model_source,
    detail::empty_unloader,  // detail::unload_model,
    detail::empty_unloader,  // detail::unload_animation,
    detail::unload_script_project,
    detail::unload_script_source,
    detail::unload_script_file,
    detail::unload_script,
    detail::empty_unloader,  // detail::unload_audio,
    detail::unload_scene,
    detail::empty_unloader,  // detail::unload_input_map,
    detail::unload_rendering_pipeline,
    detail::unload_asset_declaration,
    detail::empty_unloader,
  };

  bool asset_pipeline::is_extension_supported(const std::string_view extension) {
    return std::ranges::find_if(kAssetExtensions, [extension](const auto& ext) { return ext.extension == extension; }) != kAssetExtensions.end();
  }

  scope<asset_pipeline> asset_pipeline::get_asset_pipeline(event_system* events, asset_handler* handler, asset::type type) {
    switch (type) {
      case asset::MODEL_SOURCE: return make_scope<model_source_pipeline>(events, handler);
      case asset::SCRIPT_PROJECT: return make_scope<script_project_pipeline>(events, handler);
      case asset::SCRIPT_SOURCE: return make_scope<script_source_pipeline>(events, handler);
      case asset::SCRIPT_FILE: return make_scope<script_file_pipeline>(events, handler);
      case asset::SCRIPT: return make_scope<script_pipeline>(events, handler);
      case asset::SCENE: return make_scope<scene_pipeline>(events, handler, nullptr);
      case asset::RENDERING_PIPELINE: return make_scope<rendering_pipeline_pipeline>(events, handler, pipeline_definition{});
      case asset::ASSET_DECLARATION: return make_scope<asset_declaration_pipeline>(events, handler);
      default:
        OTHER_ASSERT(false, "No asset pipeline for asset type {}", type);
    }
  }

  scope<asset_pipeline> asset_pipeline::get_model_source_pipeline(event_system* events, asset_handler* handler, const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    CORE_LOG_DEBUG("Building model source pipeline for model '{}', vertex count {}, index count {}", name, vertices.size(), indices.size());
    scope<model_source_pipeline> pl = make_scope<model_source_pipeline>(events, handler);
    pl->builder = model_importer::build_model_data(name, vertices, indices);
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

  void asset_pipeline::start_load(executor_t& execution_pool, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
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
    start_load_operation(
      execution_pool, asset_ptr, on_success, on_failure,
      loading_table::loaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::start_unload(executor_t& execution_pool, asset* asset_ptr, asset_pipeline::on_asset_loaded on_success, asset_pipeline::on_asset_load_failed on_failure) {
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
    start_load_operation(
      execution_pool, asset_ptr, on_success, on_failure,
      loading_table::unloaders[asset_ptr->asset_type]
    );
  }

  void asset_pipeline::poll() {
    if (!pipeline_state.loading && !pipeline_state.unloading) {
      return;
    }

    if (pipeline_state.success) {
      pipeline_complete(asset_ptr);
    } else if (pipeline_state.failure) {
      std::string error_message;
      pipeline_failed(asset_ptr, error_message);
    }

    if (pipeline_state.success || pipeline_state.failure) {
      reset();
    }
  }

  void asset_pipeline::start_load_operation(executor_t& execution_pool, asset* asset_ptr, on_asset_loaded on_success, on_asset_load_failed on_failure, loading_table::loader_fn_t function) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null");
    OTHER_ASSERT(function != nullptr, "Loader function pointer is null");
    OTHER_ASSERT(handler != nullptr, "Asset handler pointer is null");

    on_success_callback = on_success;
    on_failure_callback = on_failure;
    this->asset_ptr = asset_ptr;

    CORE_LOG_TRACE("Posting asset load operation for asset: [{}]", asset_ptr->id);
    handler->jobs.post_coroutine(function(handler, asset_ptr, &asset_pipeline::pipeline_finished, &asset_pipeline::pipeline_failed, this));
  }

  void asset_pipeline::pipeline_finished() {
    CORE_LOG_DEBUG("Pipeline finished successfully");
    pipeline_state.success = true;
  }

  void asset_pipeline::pipeline_failed(const std::string& err_msg) {
    error_message = err_msg;
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
    OTHER_ASSERT(pipeline_state.loading || pipeline_state.unloading, "Pipeline is not in loading or unloading state in pipeline_complete");
    CORE_LOG_TRACE("Pipeline complete for asset ID: {}", asset_ptr->id);

    std::string event_name = "";
    if (pipeline_state.loading) {
      event_name = "asset-loaded";
    } else if (pipeline_state.unloading) {
      event_name = "asset-unloaded";
    }

    get_events().trigger_event(get_asset_event_name(asset_ptr->asset_type, event_name), asset_ptr->id);

    /// \todo wire events and remove this
    if (on_success_callback != nullptr) {
      on_success_callback(asset_ptr);
    }
  }

  void asset_pipeline::pipeline_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in pipeline_failed");
    OTHER_ASSERT(pipeline_state.loading || pipeline_state.unloading, "Pipeline is not in loading or unloading state in pipeline_failed");

    CORE_LOG_ERROR("Pipeline failed for asset ID: {}", asset_ptr->id);
    CORE_LOG_ERROR(" - Error message: {}", error_message);

    std::string event_name = "";
    if (pipeline_state.loading) {
      event_name = "asset-load-failed";
    } else if (pipeline_state.unloading) {
      event_name = "asset-unload-failed";
    }
    get_events().trigger_event(get_asset_event_name(asset_ptr->asset_type, event_name), std::make_tuple(asset_ptr->id, error_message));

    /// \todo wire events and remove this
    if (on_failure_callback != nullptr) {
      on_failure_callback(asset_ptr, error_message);
    }
  }

  // task asset_pipeline::load_asset(asset* asset_ptr) {
  //   OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load_asset");

  //   opt<filepath> load_path = get_asset_load_path(asset_ptr);
  //   opt<job::descriptor> job_desc = get_asset_load_job_descriptor(asset_ptr, load_path);

  //   ref<job> load_job = nullptr;
  //   if (job_desc.has_value()) {
  //     // source_job = handler->get_job_system().submit(job_desc.value(), std::bind_front(&detail::load_model_from_path, std::ref(builder), load_path.value()));

  //     /// yield if we posted the job for the worker thread to pick it up
  //     co_await task::yield();
  //   }

  //   std::vector<natural_t> dependencies = {};
  //   if (load_job != nullptr) {
  //     /// it is probably done here since we yielded after posting it,
  //     //   but just in case we can yield again until it is done
  //     co_await task::wait_for_job(load_job);
  //     if (load_job->get_status() != job::status::COMPLETED) {
  //       // builder = {};
  //     }

  //     dependencies.push_back(load_job->id);
  //   } else {
  //     // builder = reinterpret_cast<model_source_pipeline*>(pipeline)->builder;
  //   }

  //   co_return;
  // }

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

      model_builder builder;
      std::vector<natural_t> dependencies = {};

      ref<job> source_job = nullptr;
      if (load_model) {
        OTHER_ASSERT(std::filesystem::exists(source_path), "Model source file does not exist: {}", source_path.string());
        CORE_LOG_DEBUG("Loading model source from file: {}", source_path.string());

        source_job = handler->get_job_system().submit(
          {
            .name = std::format("Load Model Source Asset {}", asset_ptr->id),
            .priority = job::priority::LOW,
            /// this should run on worker thread since it could take aribitrarily long
            .thread_affinity = job::affinity::WORKER_THREAD,
          },
          [&builder, source_path]() {
            /// this is fine to leave unprotected by a mutex because the job system garuantees this won't be touched
            ///  until first job finishes and since it is local to the coroutine loading it there won't be any concurrent access to it
            builder = model_importer::load_model_data(source_path);
          }
        );

        dependencies.push_back(source_job->id);
      } else {
        CORE_LOG_DEBUG(" - finalizing model upload for model {}", asset_ptr->virtual_path);
        asset_ptr->path_hash = FNV(asset_ptr->virtual_path);
        builder = std::move(reinterpret_cast<model_source_pipeline*>(pipeline)->builder);
      }

      auto store_job = handler->get_job_system().submit(
        {
          .name = std::format("Finalize Load Model Source Asset {}", asset_ptr->id),
          .priority = job::priority::LOW,
          /// this job runs on main thread because it is gonna upload to the gpu
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [asset_ptr, b = &builder]() {
          OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in finalize load model source job");
          if (b->vertices.empty() || b->indices.empty() || b->submeshes.empty() || b->nodes.empty()) {
            throw std::runtime_error(std::format("Failed to load model source asset: {} (data invalid)", asset_ptr->load_path.string()));
          }

          std::string name = asset_ptr->load_path.filename().stem().string();
          if (name.empty()) {
            OTHER_ASSERT(b->name.has_value(), "Model builder name is not set for model source asset with empty filename");
            name = b->name.value();
          }

          // clang-format off
          ref<model_source> src = make_ref<model_source>(name, b->vertices,  b->indices, b->triangles, b->submeshes, b->nodes, b->materials, 
                                                         b->animations, b->skel, b->global_transform, b->inverse_global_transform, b->bounds);
          // clang-format on
          if (!src) {
            CORE_LOG_ERROR("Failed to create model source for file: {}", asset_ptr->load_path.string());
            return;
          }

          subsystem<renderer_backend>::get()->add_model_source(asset_ptr->path_hash, src);
          CORE_LOG_INFO("Model source loaded and registered: {} with hash {}", asset_ptr->load_path.string(), asset_ptr->path_hash);
        },
        dependencies
      );

      /// waiting on this will also wait on the first job
      do {
        co_await task::yield();
      } while (!store_job->done());
      OTHER_ASSERT(store_job->done(), "Store job for model source asset in invalid state. Status: {}", store_job->get_status());

      if (store_job->get_status() == job::status::COMPLETED) {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_success);
      } else {
        call_pipeline_fn<model_source_pipeline>(pipeline, on_failure, std::format("Failed to finalize model source asset: {}", source_path.string()));
      }
    }

    task load_script_project(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);

      filepath project_path = asset_ptr->absolute_path;

      //  launch all project files since they are simple and are only registered in the correct place,
      //  if does not exist, then path is generated in the build job below
      if (std::filesystem::exists(project_path)) {
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(project_path.parent_path(), ec);
             it != std::filesystem::recursive_directory_iterator();
             it.increment(ec)) {
          // skip build files/generated files
          if (it->is_directory() && (it->path().filename() == "obj" || it->path().filename() == "bin")) {
            it.disable_recursion_pending();
            continue;
          }

          if (it->is_regular_file() && it->path().extension() == ".cs") {
            handler->load_asset(it->path());
          }
        }
      }

      auto& jobs = handler->get_job_system();

      ref<project_tool> build_tool = make_ref<project_tool>();
      natural_t build_id = 0;
      {
        ref<job> build_project_job = jobs.submit(
          {
            .name = std::format("Build .NET project '{}'", project_path.string()),
            .priority = job::priority::LOW,
            .thread_affinity = job::affinity::WORKER_THREAD,
          },
          [t = build_tool, path = project_path]() mutable {
            OTHER_ASSERT(t != nullptr, "Failed to create project tool for building .NET project.");
            /// create dotnet project for the loaded project
            if (!std::filesystem::exists(path)) {
              CORE_LOG_INFO("No .NET project file found at '{}', creating a new one.", path.string());
              t->generate_dotnet_project(path);
            }

            /// .csproj file exists we go straight to building it
            t->start_project_build(path);

            do {
              std::this_thread::yield();
            } while (t->project_build_in_progress());

            int32_t result = t->get_build_result();
            t->cleanup_build();

            if (result == 0) {
              CORE_LOG_INFO("Successfully built .NET project '{}'", path.string());
            } else {
              throw std::runtime_error(std::format("Failed to build .NET project '{}'. Build result code: {}", path.string(), result));
            }
          }
        );
        OTHER_ASSERT(build_project_job != nullptr, "Failed to create job for building .NET project.");
        build_id = build_project_job->id;
      }

      ref<job> load_build_asset_job = jobs.submit_deferred(
        build_id,
        {
          .name = std::format("Load built assembly for .NET project '{}'", project_path.string()),
          .priority = job::priority::LOW,
          .thread_affinity = job::affinity::MAIN_THREAD,
        },
        [h = handler, t = build_tool, project_path]() {
          filepath csproj = t->get_dotnet_project_path();
          /// \todo fixed hardcoded build configuration and output path assumptions
          filepath build = csproj.parent_path() / "bin" / "Debug" / (csproj.stem().string() + ".dll");
          if (!std::filesystem::exists(build)) {
            throw std::runtime_error(std::format("Expected built assembly '{}' does not exist.", build.string()));
          }

          h->load_asset(build);
        }
      );
      OTHER_ASSERT(load_build_asset_job != nullptr, "Failed to create job for loading built assembly of .NET project.");

      do {
        co_await task::yield();
      } while (!load_build_asset_job->done());

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

      scene* scene_ptr = nullptr;
      if (asset_ptr->path_hash == 0) {
        asset_ptr->path_hash = FNV(asset_ptr->virtual_path);
        scene_ptr = reinterpret_cast<scene_pipeline*>(pipeline)->scene_ptr;
        scene_ptr->asset_id = asset_ptr->id;
      } else {
        OTHER_ASSERT(std::filesystem::exists(asset_ptr->absolute_path), "Scene file does not exist: {}", asset_ptr->absolute_path.string());
        CORE_LOG_DEBUG("Loading scene from file: {}", asset_ptr->load_path.string());

        /**
         * \todo load scene from file if binary file attached
         **/

        co_await task::yield();
      }

      CORE_LOG_DEBUG("Scene [{}] loaded successfully, running scene scripts if any.", scene_ptr->id);
      scene_ptr->asset_id = asset_ptr->id;
      call_pipeline_fn<scene_pipeline>(pipeline, on_success);
      co_return;
    }

    task load_rendering_pipeline(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      call_pipeline_fn<rendering_pipeline_pipeline>(pipeline, on_success);
      co_return;
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

    task empty_loader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      OTHER_ASSERT(false, "No loader implemented for asset type {} in empty_loader", asset_ptr->asset_type);
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

    task empty_unloader(asset_handler* handler, asset* asset_ptr, asset_pipeline::on_load_success_fn on_success, asset_pipeline::on_load_failure_fn on_failure, void* pipeline) {
      verify_parameters(handler, asset_ptr, on_success, on_failure, pipeline);
      OTHER_ASSERT(false, "No loader implemented for asset type {} in empty_loader", asset_ptr->asset_type);
      co_return;
    }

  }  // namespace detail
}  // namespace other