/**
 * \file driver/systems/rendering_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP

#include "core/scope.hpp"

#include "renderer/pass_executor_resolver.hpp"
#include "renderer/renderer.hpp"

#include "driver/driver_kernel.hpp"
#include "driver/systems/core_system.hpp"
#include "ui/driver_ui.hpp"

namespace other {

  class OTHER_CLASS rendering_system : public core_system<rendering_system> {
   public:
    rendering_system(driver* driver_instance)
        : core_system<rendering_system>(driver_instance, static_cast<uint32_t>(driver_system_type::RENDERING_DRIVER_SYSTEM)) {}

    std::string name() const override { return "Rendering System"; }

    void initialize(driver_kernel* kernel) override;
    void late_initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    void render(driver_kernel* kernel);

    void open_ui_window(const std::string_view name);
    void close_ui_window(const std::string_view name);

    ui::menu build_menu(const std::string_view name, const sol::table& menu_table);
    ui::menu_item build_menu_item(const std::string_view name, sol::function action);

    scope<renderer>& get_renderer();
    scope<driver_ui>& get_driver_ui();

    using file_dialog_callback_fn = void (*)(void* userdata, const char* const* filelist, int32_t filter);
    void show_open_file_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props);
    void show_open_folder_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props);
    void show_save_file_dialog(file_dialog_callback_fn callback_fn, void* user_data, uint32_t props);

   private:
    scope<renderer> renderer_ptr = nullptr;
    scope<pass_executor_resolver> pass_resolver_ptr = nullptr;
    scope<driver_ui> driver_ui_ptr = nullptr;
    glm::vec2 viewport_size = { 0.0f, 0.0f };

    std::vector<natural_t> pending_rendering_pipeline_assets;
    std::vector<natural_t> unloading_rendering_pipeline_assets;
    std::vector<natural_t> rendering_pipeline_assets;

    void register_builtin_resource_tags();
    void register_builtin_render_executors();
    void register_buildin_renderer_debug_streams();

    void configure_pipelines(driver_kernel* kernel);

    void handle_viewport_resize_event(const value& data);
    void handle_ls_windows_event(driver_kernel* kernel, const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP