/**
 * \file asset/asset_handler.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_HANDLER_HPP
#define OTHER_SCENE_ASSET_ASSET_HANDLER_HPP

#include <deque>
#include <queue>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/state_machine.hpp"

#include "asset/asset.hpp"

namespace other {

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

    void purge_stores();

    natural_t load_asset(const filepath& file_path);
    void unload_asset(natural_t asset_id);

    asset_state get_asset_state(natural_t asset_id) const;

    size_t get_num_loading_assets() const { return loading_assets.size(); }
    size_t get_num_loaded_assets() const { return loaded_assets.size(); }
    size_t get_num_assets_in_flight() const { return loading_assets.size() + loaded_assets.size(); }

    size_t get_num_pending_unloads() const { return pending_unloads.size(); }
    // size_t process_pending_unloads(size_t max_to_process = SIZE_MAX);

   private:
    asio::io_context& io_context;

    std::deque<asset> loading_assets;
    std::unordered_map<natural_t, asset> loaded_assets;
    std::unordered_map<natural_t, asset_state_machine> asset_states;

    std::queue<natural_t> pending_unloads;

    struct loading_table {
      using loader_fn_t = std::function<void(asset*, std::function<void()>, std::function<void(const std::string&)>)>;
      static std::array<loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> loaders;
      static std::array<loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> unloaders;
    };

    static inline natural_t next_asset_id = 1;
    static inline natural_t get_next_asset_id() {
      return next_asset_id++;
    }

    void on_asset_loaded(asset* asset_ptr);
    void on_asset_load_failed(asset* asset_ptr, const std::string& error_message);

    void on_asset_unloaded(asset* asset_ptr);
    void on_asset_unload_failed(asset* asset_ptr, const std::string& error_message);
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HANDLER_HPP