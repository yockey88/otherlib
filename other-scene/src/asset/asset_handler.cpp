/**
 * \file asset/asset_handler.cpp
 **/
#include "asset/asset_handler.hpp"

#include <ranges>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "serialization/serialization.hpp"

#include "model/model_importer.hpp"
#include "renderer/renderer_backend.hpp"

#include "asio/asio/async_result.hpp"
#include "asio/asio/compose.hpp"

namespace other {

  namespace detail {

    void load_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void load_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void load_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void load_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void load_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void empty_loader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);

    void unload_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void unload_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void unload_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void unload_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void unload_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);
    void empty_unloader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure);

  }  // namespace detail

  std::array<asset_handler::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_handler::loading_table::loaders = {
    detail::load_texture,
    detail::load_model_source,
    detail::load_model,
    detail::load_script_source,
    detail::load_audio,
    detail::empty_loader,
  };

  std::array<asset_handler::loading_table::loader_fn_t, static_cast<size_t>(asset::NUM_ASSET_TYPES)> asset_handler::loading_table::unloaders = {
    detail::unload_texture,
    detail::unload_model_source,
    detail::unload_model,
    detail::unload_script_source,
    detail::unload_audio,
    detail::empty_unloader,
  };

  void asset_handler::purge_stores() {
    while (!pending_unloads.empty()) {
      natural_t asset_id = pending_unloads.front();
      pending_unloads.pop();

      auto it = loaded_assets.find(asset_id);
      if (it != loaded_assets.end()) {
        loaded_assets.erase(it);
        CORE_LOG_DEBUG("Purged asset from store (ID: {})", asset_id);
      }
    }
  }

  natural_t asset_handler::load_asset(const filepath& file_path) {
    natural_t asset_id = get_next_asset_id();

    if (!std::filesystem::exists(file_path)) {
      CORE_LOG_ERROR("Asset file does not exist: {}", file_path.string());
      return 0;
    }

    if (auto itr = std::ranges::find_if(loaded_assets, [&file_path](const auto& pair) { return pair.second.path_hash == FNV(file_path.string()); });
        itr != loaded_assets.end()) {
      CORE_LOG_WARN("Attempting to reload asset: {}", file_path.string());
      return itr->second.id;
    }
    if (auto itr = std::ranges::find_if(loading_assets, [&file_path](const auto& pair) { return pair.path_hash == FNV(file_path.string()); });
        itr != loading_assets.end()) {
      CORE_LOG_WARN("Attempting to reload asset: {}", file_path.string());
      return itr->id;
    }

    std::string extension = file_path.extension().string();
    asset::type asset_type = asset::get_type_from_extension(extension);

    if (asset_type == asset::type::EMPTY) {
      CORE_LOG_ERROR("Unsupported asset file extension: {}", extension);
      return 0;
    }

    auto it = loading_assets.insert(loading_assets.end(), asset{ asset_type, asset_id, FNV(file_path.string()), file_path });
    OTHER_ASSERT(it != loading_assets.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(asset_id, asset_state_machine{});
    if (!state_inserted) {
      CORE_LOG_ERROR("Failed to create asset state machine for asset: {}", file_path.string());
      loading_assets.erase(it);
      return 0;
    }

    asset* loading_asset = &(*it);

    state_it->second.handle_event(asset_event::LOAD_REQUESTED, loading_asset);

    /// begin async loading here
    asio::post(io_context, [loading_asset, handler = this, loader = loading_table::loaders[static_cast<size_t>(loading_asset->asset_type)]]() {
      OTHER_ASSERT(loader != nullptr, "Loader function is null for asset type {}", static_cast<size_t>(loading_asset->asset_type));
      try {
        loader(
          loading_asset,
          [handler, loading_asset]() { handler->on_asset_loaded(loading_asset); },
          [handler, loading_asset](const std::string& error_msg) { handler->on_asset_load_failed(loading_asset, error_msg); }
        );
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception occurred while loading asset: {}", e.what());
        handler->on_asset_load_failed(loading_asset, e.what());
      } catch (...) {
        CORE_LOG_ERROR("Unknown exception occurred while loading asset");
        handler->on_asset_load_failed(loading_asset, "Unknown exception");
      }
    });

    return asset_id;
  }

  void asset_handler::unload_asset(natural_t asset_id) {
    auto state_itr = asset_states.find(asset_id);
    if (state_itr == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return;
    }

    if (state_itr->second.get_current_state() != asset_state::LOADED) {
      CORE_LOG_WARN("Attempting to unload asset ID: {} which is not in LOADED state", asset_id);
      return;
    }
    CORE_LOG_DEBUG("Unloading asset ID: {}", asset_id);

    state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    auto it = loaded_assets.find(asset_id);
    OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);
    asset* loaded_asset = &it->second;

    asio::post(io_context, [loaded_asset, handler = this, unloader = loading_table::unloaders[static_cast<size_t>(loaded_asset->asset_type)]]() {
      OTHER_ASSERT(unloader != nullptr, "Unloader function is null for asset type {}", static_cast<size_t>(loaded_asset->asset_type));
      try {
        unloader(
          loaded_asset,
          [handler, loaded_asset]() { handler->on_asset_unloaded(loaded_asset); },
          [handler, loaded_asset](const std::string& error_msg) { handler->on_asset_unload_failed(loaded_asset, error_msg); }
        );
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception occurred while unloading asset: {}", e.what());
        handler->on_asset_unload_failed(loaded_asset, e.what());
      } catch (...) {
        CORE_LOG_ERROR("Unknown exception occurred while unloading asset");
        handler->on_asset_unload_failed(loaded_asset, "Unknown exception");
      }
    });
  }

  asset_state asset_handler::get_asset_state(natural_t asset_id) const {
    auto it = asset_states.find(asset_id);
    if (it != asset_states.end()) {
      return it->second.get_current_state();
    }
    return asset_state::ERROR_STATE;
  }

  void asset_handler::on_asset_loaded(asset* asset_ptr) {
    CORE_LOG_DEBUG("Asset loaded successfully (ID: {})", asset_ptr->id);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    auto pending_itr = std::ranges::find_if(loading_assets, [asset_ptr](const auto& a) { return a.id == asset_ptr->id; });
    OTHER_ASSERT(pending_itr != loading_assets.end(), "Loaded asset not found in loading assets");

    auto itr = loaded_assets.emplace(asset_ptr->id, std::move(*pending_itr));
    OTHER_ASSERT(itr.second, "Failed to insert loaded asset into loaded assets map");
    loading_assets.erase(pending_itr);

    state_itr->second.handle_event(asset_event::LOAD_COMPLETED);
  }

  void asset_handler::on_asset_load_failed(asset* asset_ptr, const std::string& error_message) {
    CORE_LOG_ERROR("Failed to load asset (ID: {}): {}", asset_ptr->id, error_message);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::ERROR_EVENT);
  }

  void asset_handler::on_asset_unloaded(asset* asset_ptr) {
    CORE_LOG_DEBUG("Asset unloaded successfully (ID: {})", asset_ptr->id);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::UNLOAD_COMPLETED);

    pending_unloads.push(asset_ptr->id);
  }

  void asset_handler::on_asset_unload_failed(asset* asset_ptr, const std::string& error_message) {
    CORE_LOG_ERROR("Failed to unload asset (ID: {}): {}", asset_ptr->id, error_message);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::ERROR_EVENT);
  }

  namespace detail {

    void load_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void load_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null");
      OTHER_ASSERT(on_success != nullptr, "on_success callback is null");
      OTHER_ASSERT(on_failure != nullptr, "on_failure callback is null");

      OTHER_ASSERT(std::filesystem::exists(asset_ptr->path), "Model source file does not exist: {}", asset_ptr->path.string());

      filepath file_path = asset_ptr->path;
      CORE_LOG_DEBUG("Loading model source from file: {}", file_path.string());

      /// \todo currently have to look up model source again after loading using filepath hash which is not intuitive
      ///       should refactor with a universal id system
      model_builder builder = model_importer::load_model_data(file_path);
      if (builder.vertices.empty() || builder.indices.empty() || builder.submeshes.empty()) {
        on_failure(std::format("Failed to load model source: {} (no valid data)", file_path.string()));
        return;
      }

      {
        PROFILE_SECTION("model_importer::load_model_data--create-model-source");
        auto now = std::chrono::high_resolution_clock::now();
        ref<model_source> src = make_ref<model_source>(file_path.filename().stem().string(), builder.vertices, builder.indices, builder.triangles, builder.submeshes, builder.nodes, builder.bounds);
        if (!src) {
          on_failure(std::format("Failed to create model source for file: {}", file_path.string()));
          return;
        }

        natural_t hash = FNV(file_path.string());
        subsystem<renderer_backend>::get()->add_model_source(hash, src);
        CORE_LOG_DEBUG("Model source loaded and registered: {} with hash {}", file_path.string(), hash);
        auto end = std::chrono::high_resolution_clock::now();
        CORE_LOG_DEBUG("Model source creation time: {} ms", std::chrono::duration<float, std::milli>(end - now).count());
      }

      on_success();
    }

    void load_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void load_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void load_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void empty_loader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
      on_success();
    }

    void unload_texture(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void unload_model_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
      CORE_LOG_DEBUG("Unloading model source (ID: {})", asset_ptr->id);

      auto it = subsystem<renderer_backend>::get()->get_model_source(FNV(asset_ptr->path.string()));
      if (it == nullptr) {
        CORE_LOG_WARN("Model source not found for unloading: {}", asset_ptr->path.string());
        on_failure(std::format("Model source not found for unloading: {}", asset_ptr->path.string()));
        return;
      }

      subsystem<renderer_backend>::get()->remove_model_source(FNV(asset_ptr->path.string()));
      on_success();
      CORE_LOG_DEBUG("Model source unloaded successfully (ID: {})", asset_ptr->id);
    }

    void unload_model(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void unload_script_source(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void unload_audio(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
    }

    void empty_unloader(asset* asset_ptr, std::function<void()> on_success, std::function<void(const std::string&)> on_failure) {
      on_success();
    }

  }  // namespace detail
}  // namespace other