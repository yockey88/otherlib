/**
 * \file asset/asset_handler.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_HANDLER_HPP
#define OTHER_SCENE_ASSET_ASSET_HANDLER_HPP

#include <deque>
#include <queue>
#include <string_view>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/state_machine.hpp"

#include "asset/asset.hpp"
#include "asset/asset_pipeline.hpp"

namespace other {
  namespace detail {

    struct load_context;

  }  // namespace detail

  enum asset_state {
    UNLOADED = 0,
    LOADING,
    LOADED,
    OUT_OF_DATE,
    UNLOADING,

    ERROR_STATE,
    NUM_STATES = ERROR_STATE,
  };

  enum asset_event {
    LOAD_REQUESTED = 0,
    LOAD_COMPLETED,
    LOAD_FAILED,
    TIMESTAMP_UPDATED,
    UNLOAD_REQUESTED,
    UNLOAD_COMPLETED,

    ERROR_EVENT,
    NUM_EVENTS = ERROR_EVENT,
  };

  class asset_state_machine : public state_machine<asset_state, asset_event> {
   public:
    asset_state_machine()
        : state_machine<asset_state, asset_event>(asset_state::UNLOADED) {
      add_transition(asset_state::UNLOADED, asset_event::LOAD_REQUESTED, asset_state::LOADING);
      add_transition(asset_state::LOADING, asset_event::LOAD_COMPLETED, asset_state::LOADED);
      add_transition(asset_state::LOADING, asset_event::LOAD_FAILED, asset_state::ERROR_STATE);
      add_transition(asset_state::LOADED, asset_event::UNLOAD_REQUESTED, asset_state::UNLOADING);
      add_transition(asset_state::LOADED, asset_event::TIMESTAMP_UPDATED, asset_state::OUT_OF_DATE);
      add_transition(asset_state::OUT_OF_DATE, asset_event::LOAD_REQUESTED, asset_state::LOADING);
      add_transition(asset_state::UNLOADING, asset_event::UNLOAD_COMPLETED, asset_state::UNLOADED);

      /// all states to error state on error event
      for (size_t s = 0; s < static_cast<size_t>(asset_state::NUM_STATES); ++s) {
        add_transition(
          static_cast<asset_state>(s), asset_event::ERROR_EVENT, asset_state::ERROR_STATE,
          [](asset_state from, asset_event event, asset_state to, void* data) {
            CORE_LOG_ERROR("Asset in ERROR_STATE after {}", static_cast<size_t>(from));
          }
        );
      }
    }
    ~asset_state_machine() = default;
  };

  class asset_handler {
   public:
    asset_handler(asio::io_context& io_context)
        : io_context(io_context) {}
    ~asset_handler() = default;

    static std::vector<asset::type> get_convertible_asset_types(asset::type requested_type);

    void purge_stores();
    void update_pipelines();

    using load_completion_callback = std::function<void(asset*)>;
    using load_error_callback = std::function<void(asset*)>;

    //  static bool is_asset_id_field(const std::string& field_name) {
    //     /// \todo improve this by allowing users to specify which fields are asset id fields, maybe through a traits system or something
    //     /// for now we will just assume any field named "asset_id" or ending with "_asset_id" is an asset id field
    //     if (field_name == "asset_id" || field_name.ends_with("_asset_id")) {
    //       return true;
    //     }
    //     return false;
    // }

    natural_t load_asset(const filepath& file_path, load_completion_callback on_complete = nullptr);
    natural_t load_asset(const std::string_view engine_path, load_completion_callback on_complete = nullptr);
    void unload_asset(natural_t asset_id);

    /// checks if asset is ready for use
    inline bool asset_loaded(natural_t asset_id) const {
      return loaded_assets.find(asset_id) != loaded_assets.end();
    }
    /// checks if asset is currently loading
    inline bool asset_loading(natural_t asset_id) const {
      auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& a) { return a.loading_asset.id == asset_id; });
      return it != asset_pipelines.end();
    }
    /// checks if asset exists in system, usable or not
    inline bool asset_exists(natural_t asset_id) const {
      return asset_states.find(asset_id) != asset_states.end();
    }

    void set_default_mount(const std::string_view mount_name) { default_mount = mount_name; }

    asset_state get_asset_state(natural_t asset_id) const;
    asset_state get_asset_state_by_path_hash(natural_t path_hash) const;
    natural_t get_asset_hash(natural_t asset_id) const;

    const asset* get_loaded_asset(natural_t asset_id) const;
    std::vector<natural_t> get_all_tracked_ids() const;

    asio::io_context& get_io_context() { return io_context; }
    asio::thread_pool& get_thread_pool() { return thread_pool; }

    size_t get_num_loading_assets() const { return asset_pipelines.size(); }
    size_t get_num_loaded_assets() const { return loaded_assets.size(); }
    size_t get_num_assets_in_flight() const { return asset_pipelines.size() + loaded_assets.size(); }

    size_t get_num_pending_unloads() const { return pending_unloads.size(); }
    // size_t process_pending_unloads(size_t max_to_process = SIZE_MAX);

   private:
    asio::io_context& io_context;
    /// \todo figure out model-source pipeline race condition, currently two models loading at the same
    ///       time when using more than one thread causes issues
    asio::thread_pool thread_pool{ 1 };

    struct pipeline_context {
      scope<asset_pipeline> pipeline = nullptr;
      asset loading_asset;
      load_completion_callback on_complete = nullptr;
      load_error_callback on_error = nullptr;
    };
    std::deque<pipeline_context> asset_pipelines;

    std::unordered_map<natural_t, asset> loaded_assets;
    std::unordered_map<natural_t, asset_state_machine> asset_states;

    std::queue<natural_t> pending_unloads;

    std::string default_mount = "assets";

    static inline natural_t next_asset_id = 1;
    static inline natural_t get_next_asset_id() {
      return next_asset_id++;
    }

    friend struct detail::load_context;
    friend class asset_pipeline;

    void on_asset_loaded(asset* asset_ptr);
    void on_asset_load_failed(asset* asset_ptr, const std::string& error_message);

    void on_asset_unloaded(asset* asset_ptr);
    void on_asset_unload_failed(asset* asset_ptr, const std::string& error_message);

    void register_asset_in_filesystem(const asset* asset_ptr);
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HANDLER_HPP