/**
 * \file driver/systems/asset_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP

#include "core/scope.hpp"

#include "driver/driver_kernel.hpp"

#include "asset/asset_handler.hpp"

namespace other {

  class asset_system : public core_system<asset_system> {
   public:
    asset_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::ASSET_DRIVER_SYSTEM) {}
    virtual ~asset_system() = default;

    std::string name() const override { return "Asset System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    natural_t begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded = nullptr);
    natural_t add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices);
    void begin_full_unload();

    scope<asset_handler>& get_asset_manager();

    natural_t get_asset_hash(natural_t asset_id) const;

   private:
    struct loading_asset {
      using handler = std::function<void(natural_t)>;
      natural_t asset_id = 0;
      handler on_loaded = nullptr;
    };

    scope<asset_handler> asset_mgr = nullptr;
    std::deque<loading_asset> loading_asset_ids;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_ASSET_SYSTEM_HPP