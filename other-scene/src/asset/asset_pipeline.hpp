/**
 * \file asset/asset_pipeline.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP
#define OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP

#include <asio/asio.hpp>

#include "core/scope.hpp"

#include "asset/asset.hpp"

namespace other {

  class asset_handler;

  class asset_pipeline {
   public:
    asset_pipeline(asset_handler* handler)
        : handler(handler) {
      OTHER_ASSERT(handler != nullptr, "Asset handler is null in asset_pipeline");
    }
    virtual ~asset_pipeline() = default;

    static scope<asset_pipeline> get_asset_pipeline(asset::type type, asset_handler* handler);

    void start_load(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_succes, std::function<void(const std::string&)> on_failure);
    void start_unload(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_succes, std::function<void(const std::string&)> on_failure);

    void poll();

    struct loading_table {
      using loader_fn_t = std::function<void(asset*, std::function<void()>, std::function<void(const std::string&)>, void*)>;
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

    void start_load_operation(asio::thread_pool& execution_pool, asset* asset_ptr, std::function<void()> on_succes, std::function<void(const std::string&)> on_failure, loading_table::loader_fn_t function);

    void pipeline_finished();
    void pipeline_failed(const std::string& error_message);

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

    asset* asset_ptr = nullptr;

    asset_handler* handler = nullptr;

    std::function<void()> on_success_callback = nullptr;
    std::function<void(const std::string&)> on_failure_callback = nullptr;

    void pipeline_complete(asset* asset_ptr);
    void pipeline_failed(asset* asset_ptr, const std::string& error_message);
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_PIPELINE_HPP