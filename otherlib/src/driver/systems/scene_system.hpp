/**
 * \file driver/systems/scene_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP

#include "core/value.hpp"

#include "scene/scene.hpp"
#include "scene/scene_graph.hpp"

#include "driver/driver_kernel.hpp"

namespace other {

  class scene_system : public core_system<scene_system> {
   public:
    scene_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::SCENE_DRIVER_SYSTEM) {}
    virtual ~scene_system() = default;

    std::string name() const override { return "Scene Driver System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    natural_t add_scene_to_scene_graph(const filepath& scene_path);
    natural_t create_empty_scene(const std::string_view name, bool add_asset = true);
    natural_t get_id_of_scene(const std::string_view name);
    scene* get_scene(natural_t id);

    void set_scene_to_active(natural_t scene_id);
    void synchronize_active_scene(natural_t scene_id);
    void unload_active_scene();

    void push_scene_object_to_context_stack(scene_object* object);
    scene_object* pop_scene_object_from_context_stack();

    inline scene* get_active_scene() { return active_scene; }
    scene_graph& get_scene_graph();

   private:
    constexpr inline static natural_t kObjectContextStackSize = 16;
    size_t context_stack_top = 0;
    scene_object* context_stack[kObjectContextStackSize] = { nullptr };
    scene* active_scene = nullptr;
    scope<scene_graph> project_scene_graph = nullptr;

    void handle_scene_load_event(const value& data);
    void handle_scene_asset_loaded_event(const value& data);
    void handle_scene_unload_event(const value& data);
    void handle_scene_info_event(const value& data);
    void handle_scene_playback_command_event(const value& data);

    void handle_ls_scenes_event(driver_kernel* kernel, const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP