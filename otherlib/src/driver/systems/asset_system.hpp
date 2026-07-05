/**
 * \file driver/systems/asset_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP

#include "core/scope.hpp"

#include "renderer/pipeline_definition.hpp"

#include "driver/systems/core_system.hpp"

#include "asset/asset_handler.hpp"

namespace other {

  class scene;

  class OTHER_CLASS asset_system : public core_system<asset_system> {
   public:
    asset_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::ASSET_DRIVER_SYSTEM) {}
    virtual ~asset_system() = default;

    std::string name() const override { return "Asset System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    natural_t begin_asset_load(const filepath& asset_path);
    void begin_asset_unload(natural_t asset_id);

    natural_t add_model_source_asset(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices);
    natural_t add_scene_asset(scene* scene_ptr, opt<filepath> scene_path = std::nullopt);
    natural_t add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition);
    void begin_full_unload();

    asset* get_asset(natural_t asset_id);
    asset* get_asset_by_virtual_path(const filepath& virtual_path);
    /// to be called only inside 'assets.new-asset-(un)loaded' or various 'xxx.asset-(un)loaded' events.
    asset_handler::pipeline_context* get_asset_pipeline_context(natural_t asset_id);

    scope<asset_handler>& get_asset_manager();

    natural_t get_asset_hash(natural_t asset_id) const;
    natural_t get_asset_state(natural_t asset_id) const;
    natural_t get_asset_id_from_path(const filepath& path) const;

    opt<filepath> get_local_asset_path(natural_t asset_id) const;
    opt<filepath> get_virtual_asset_path(natural_t asset_id) const;

    inline bool is_asset_extension(const std::string_view extension) const {
      return asset_mgr->is_asset_extension(extension);
    }

    std::vector<asset*> get_assets_of_type(asset::type type) const;

   private:
    scope<asset_handler> asset_mgr = nullptr;
    std::deque<natural_t> loading_asset_ids;

    void mount_mounts(driver_kernel* kernel);

    void handle_ls_event(driver_kernel* kernel, const value& data);
    void handle_ls_assets_event(driver_kernel* kernel, const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP