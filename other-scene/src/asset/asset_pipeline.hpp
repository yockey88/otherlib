/**
 * \file asset/asset_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP
#define OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP

#include <asio/asio.hpp>

#include "core/scope.hpp"
#include "event/event_system.hpp"

#include "renderer/pipeline_definition.hpp"

#include "asset/asset.hpp"

namespace other {

  class asset_handler;
  class scene;

  class asset_pipeline {
   public:
    using executor_t = asio::strand<asio::io_context::executor_type>;

    using on_asset_loaded = std::function<void(asset*)>;
    using on_asset_load_failed = std::function<void(asset*, const std::string&)>;

    using on_load_success_fn = void (asset_pipeline::*)();
    using on_load_failure_fn = void (asset_pipeline::*)(const std::string&);

    asset_pipeline(event_system& events, asset_handler* handler)
        : events(events), handler(handler) {
      OTHER_ASSERT(handler != nullptr, "Asset handler is null in asset_pipeline");
    }
    virtual ~asset_pipeline() = default;

    static scope<asset_pipeline> get_asset_pipeline(event_system& events, asset_handler* handler, asset::type type);
    static scope<asset_pipeline> get_model_source_pipeline(event_system& events, asset_handler* handler, const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);
    static scope<asset_pipeline> get_scene_pipeline(event_system& events, asset_handler* handler, scene* scene_ptr);
    static scope<asset_pipeline> get_rendering_pipeline_pipeline(event_system& events, asset_handler* handler, const pipeline_definition& definition);

    void set_asset_data(asset* asset_ptr);
    void start_load(executor_t& execution_pool, asset* asset_ptr, on_asset_loaded on_success, on_asset_load_failed on_failure);
    void start_unload(executor_t& execution_pool, asset* asset_ptr, on_asset_loaded on_success, on_asset_load_failed on_failure);

    std::string get_last_error() const { return error_message; }

    void poll();

    struct loading_table {
      using loader_fn_t = std::function<void(asset*, on_load_success_fn, on_load_failure_fn, void*)>;
      static std::array<loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> loaders;
      static std::array<loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> unloaders;
    };

    std::mutex mtx;

   protected:
    virtual void on_load_complete(asset* asset_ptr) = 0;
    virtual void on_load_cancel() {}
    virtual void on_load_failed(asset* asset_ptr, const std::string& error_msg) = 0;

    virtual void on_unload_complete(asset* asset_ptr) = 0;
    virtual void on_unload_cancel() {}
    virtual void on_unload_failed(asset* asset_ptr, const std::string& error_msg) = 0;

    virtual void on_pipeline_poll() = 0;

    void start_load_operation(executor_t& execution_pool, asset* asset_ptr, on_asset_loaded on_success, on_asset_load_failed on_failure, loading_table::loader_fn_t function);

    void pipeline_finished();
    void pipeline_failed(const std::string& error_message);

    event_system& get_events() { return events; }
    asset_handler& get_handler() { return *handler; }

    void reset();

   private:
    struct flags {
      std::atomic<bool> success = false;
      std::atomic<bool> failure = false;

      std::atomic<bool> loading = false;
      std::atomic<bool> unloading = false;
    } pipeline_state;

    std::string error_message = "";

    event_system& events;

    asset* asset_ptr = nullptr;

    asset_handler* handler = nullptr;

    on_asset_loaded on_success_callback = nullptr;
    on_asset_load_failed on_failure_callback = nullptr;

    void pipeline_complete(asset* asset_ptr);
    void pipeline_failed(asset* asset_ptr, const std::string& error_message);
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP