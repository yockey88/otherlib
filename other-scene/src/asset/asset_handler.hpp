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
#include "core/job_system.hpp"
#include "core/state_machine.hpp"
#include "event/event_system.hpp"
#include "file/file_watcher.hpp"

#include "renderer/pipeline_definition.hpp"

#include "asset/asset.hpp"
#include "asset/asset_pipeline.hpp"
#include "asset/asset_resolver.hpp"

namespace other {
  namespace detail {

    static inline int32_t get_concurrency_limit() {
      int32_t concurrency = std::thread::hardware_concurrency();
      return concurrency > 0 ? concurrency : 4;
    }

  }  // namespace detail

  class scene;

  enum asset_state {
    LOADING = 0,
    LOADED,

    OUT_OF_DATE,
    REFRESHING_UNLOAD,
    REFRESHING_LOAD,

    UNLOADING,
    UNLOADED,

    ERROR_STATE,

    INVALID_STATE,
    NUM_STATES = INVALID_STATE,
  };

  enum asset_event {
    LOAD_REQUESTED = 0,
    LOAD_COMPLETED,
    LOAD_FAILED,

    TIMESTAMP_UPDATED,
    REFRESH_REQUESTED,
    REFRESH_COMPLETED,
    REFRESH_FAILED,

    UNLOAD_REQUESTED,
    UNLOAD_COMPLETED,
    UNLOAD_FAILED,

    NUM_EVENTS,
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
      add_transition(asset_state::LOADED, asset_event::REFRESH_REQUESTED, asset_state::REFRESHING_UNLOAD);

      add_transition(asset_state::OUT_OF_DATE, asset_event::REFRESH_REQUESTED, asset_state::REFRESHING_UNLOAD);
      add_transition(asset_state::OUT_OF_DATE, asset_event::UNLOAD_REQUESTED, asset_state::UNLOADING);

      add_transition(asset_state::REFRESHING_UNLOAD, asset_event::UNLOAD_COMPLETED, asset_state::REFRESHING_LOAD);
      add_transition(asset_state::REFRESHING_UNLOAD, asset_event::UNLOAD_FAILED, asset_state::ERROR_STATE);

      add_transition(asset_state::REFRESHING_LOAD, asset_event::LOAD_REQUESTED, asset_state::LOADING);
      add_transition(asset_state::REFRESHING_LOAD, asset_event::LOAD_FAILED, asset_state::ERROR_STATE);

      add_transition(asset_state::UNLOADING, asset_event::UNLOAD_COMPLETED, asset_state::UNLOADED);
      add_transition(asset_state::UNLOADING, asset_event::UNLOAD_FAILED, asset_state::ERROR_STATE);
    }
    ~asset_state_machine() = default;
  };

  class asset_handler {
   public:
    asset_handler(event_system& events, job_system& jobs, const std::string_view asset_mount = "assets")
        : events(events), jobs(jobs), default_mount(asset_mount) {
    }
    ~asset_handler() = default;

    static ostd::vector<asset::type> get_convertible_asset_types(asset::type requested_type);

    job_system& get_job_system() { return jobs; }

    bool idle() const { return asset_pipelines.empty(); }
    bool empty() const { return all_assets.empty() && idle(); }

    void begin_unload();
    void update_pipelines();

    using load_completion_callback = std::function<void(asset*)>;
    using load_error_callback = std::function<void(asset*)>;

    void resolve_roots(std::span<const filepath> roots);
    void re_resolve(const filepath& changed);

    natural_t load_asset(const filepath& file_path, load_completion_callback on_complete = nullptr);
    natural_t load_asset(const std::string_view engine_path, load_completion_callback on_complete = nullptr);
    natural_t add_model_source_asset(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices);
    natural_t add_scene_asset(scene* scene_ptr, opt<filepath> scene_path = std::nullopt);
    natural_t add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition);
    void unload_asset(natural_t asset_id);

    void reload_asset(natural_t asset_id);

    asset* get_asset(natural_t asset_id);
    asset* get_asset_by_virtual_path(const filepath& virtual_path);
    ostd::vector<asset*> get_assets_of_type(asset::type type);

    std::span<const natural_t> get_all_asset_ids() const;

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

    inline bool all_assets_unloaded() const {
      return loaded_assets.empty() && asset_pipelines.empty();
    }

    void set_default_mount(const std::string_view mount_name) { default_mount = mount_name; }

    asset_state get_asset_state(natural_t asset_id) const;
    asset_state get_asset_state_by_path_hash(natural_t path_hash) const;
    natural_t get_asset_hash(natural_t asset_id) const;
    natural_t get_asset_id_from_path(const filepath& path) const;
    natural_t get_asset_id_by_path_hash(natural_t path_hash) const;
    opt<filepath> get_local_asset_path(natural_t asset_id) const;
    opt<filepath> get_virtual_asset_path(natural_t asset_id) const;

    const asset* get_loaded_asset(natural_t asset_id) const;
    ostd::vector<natural_t> get_all_tracked_ids() const;

    void execute_plan();
    void dispatch_slot(uint32_t slot);

    void on_planned_child_loaded(natural_t stable_id);
    void on_planned_child_failed(natural_t stable_id, const std::string_view error_msg);

    size_t get_num_loading_assets() const { return asset_pipelines.size(); }
    size_t get_num_loaded_assets() const { return loaded_assets.size(); }
    size_t get_num_assets_in_flight() const { return asset_pipelines.size() + loaded_assets.size(); }

    size_t get_num_pending_unloads() const { return pending_unloads.size(); }

    inline bool is_asset_extension(const std::string_view extension) const {
      return asset_pipeline::is_extension_supported(extension);
    }

    struct pipeline_context {
      scope<asset_pipeline> pipeline = nullptr;
      asset loading_asset;
      load_completion_callback on_complete = nullptr;
      load_error_callback on_error = nullptr;
    };

    /// to be called only inside 'assets.new-asset-(un)loaded' or various 'xxx.asset-(un)loaded' events.
    asset_handler::pipeline_context* get_asset_pipeline_context(natural_t asset_id);

    inline natural_t runtime_id(natural_t stable_id) const {
      const auto it = runtime_by_stable.find(stable_id);
      return it != runtime_by_stable.end() ?
        it->second :
        0;
    }

    inline bool in_snapshot(natural_t stable_id) const {
      return snapshot.find(stable_id) != nullptr;
    }

    inline std::span<const manifest_domain> manifest_domains() const {
      return resolver.manifest_domains();
    }

   private:
    friend class asset_pipeline;

    struct load_plan {
      ostd::vector<uint32_t> remaining_children;
      ostd::vector<bool> failed;
      ostd::vector<bool> pending;
    };
    load_plan plan;

    asset_resolver resolver;
    dependency_snapshot snapshot;

    event_system& events;
    job_system& jobs;

    std::deque<pipeline_context> asset_pipelines;
    std::queue<natural_t> successful_pipelines;
    std::queue<natural_t> failed_pipelines;

    ostd::vector<natural_t> all_assets;
    ostd::unordered_map<natural_t, asset> loaded_assets;
    ostd::unordered_map<natural_t, asset> unloaded_assets;
    ostd::unordered_map<natural_t, asset_state_machine> asset_states;

    std::unordered_set<natural_t> needs_refresh;
    // stable_id -> id
    ostd::unordered_map<natural_t, natural_t> runtime_by_stable;

    /// normally we might want to recreate, but if we are closing the editor
    // or doing
    bool remove_after_unload = false;
    std::queue<natural_t> pending_loads;
    std::queue<natural_t> pending_unloads;

    std::string default_mount = "assets";

    static inline natural_t next_asset_id = 1;
    static inline natural_t get_next_asset_id() {
      return next_asset_id++;
    }

    void begin_load(std::deque<pipeline_context>::iterator pipeline_it, ostd::unordered_map<natural_t, asset_state_machine>::iterator state_it);
    ostd::unordered_map<natural_t, asset>::iterator begin_unload(natural_t asset_id);

    asset* find_asset_by_path(const filepath& file_path) const;

    void notify_asset_load_complete(asset* asset_ptr);
    void notify_asset_load_failed(asset* asset_ptr, const std::string_view error_message);
    void notify_asset_unload_complete(asset* asset_ptr);
    void notify_asset_unload_failed(asset* asset_ptr, const std::string_view error_message);

    void on_asset_loaded(natural_t id);
    void on_asset_load_failed(natural_t id);

    void on_asset_unloaded(natural_t id);
    void on_asset_unload_failed(natural_t id);

    void register_asset_in_filesystem(const asset* asset_ptr);
    void unregister_asset_in_filesystem(const asset* asset_ptr);
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HANDLER_HPP