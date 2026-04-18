/**
 * \file driver/systems/asset_system.cpp
 **/
#include "driver/systems/asset_system.hpp"

#include "driver/driver_mounts.hpp"
#include "driver/systems/network_system.hpp"

namespace other {

  void asset_system::initialize(driver_kernel* kernel) {
    auto& network = kernel->get_core_system<network_system>();
    asset_mgr = make_scope<asset_handler>(network.io_context());
    asset_mgr->set_default_mount(std::string(driver_mounts::kAssetMount));
  }

  void asset_system::tick(driver_kernel* kernel, double dt) {
    asset_mgr->update_pipelines();
  }

  void asset_system::shutdown(driver_kernel* kernel) {
    asset_mgr->purge_stores();
    asset_mgr = nullptr;
  }

  natural_t asset_system::begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    CORE_LOG_DEBUG("Loading asset at path: {}", asset_path.string());

    natural_t asset_id = asset_mgr->load_asset(asset_path, [this, asset_path](asset* asset_ptr) {
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null.");

      auto it = std::ranges::find_if(loading_asset_ids, [asset_ptr](const auto& entry) {
        return entry.asset_id == asset_ptr->id;
      });
      OTHER_ASSERT(it != loading_asset_ids.end(), "Loading asset ID not found in tracking list.");
      CORE_LOG_DEBUG("Asset loaded callback for asset ID: {} @ path: {} (virtual path: {})", asset_ptr->id, asset_path.string(), asset_ptr->virtual_path);

      loading_asset_ids.erase(it);
      // get_driver().get_event_system()->trigger_event("assets.new-asset-loaded", asset_ptr->id);
    });

    loading_asset_ids.push_back({
      .asset_id = asset_id,
      .on_loaded = on_loaded,
    });

    return asset_id;
  }

  natural_t asset_system::add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_model_source_asset(name, vertices, indices);
  }

  void asset_system::begin_full_unload() {
    asset_mgr->purge_stores();
  }

  scope<asset_handler>& asset_system::get_asset_manager() {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr;
  }

  natural_t asset_system::get_asset_hash(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_hash(asset_id);
  }

}  // namespace other