/**
 * \file driver/systems/rendering_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP

#include "core/scope.hpp"

#include "renderer/renderer.hpp"

#include "driver/driver_kernel.hpp"
#include "ui/driver_ui.hpp"

namespace other {

  class rendering_system : public core_system<rendering_system> {
   public:
    rendering_system(driver* driver_instance)
        : core_system<rendering_system>(driver_instance, static_cast<uint32_t>(driver_system_type::RENDERING_DRIVER_SYSTEM)) {}

    std::string name() const override { return "Rendering System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    void render(driver_kernel* kernel);
    void render_ui(driver_kernel* kernel);

    void open_ui_window(const std::string_view name);
    void close_ui_window(const std::string_view name);

    scope<renderer>& get_renderer();
    scope<driver_ui>& get_driver_ui();

   private:
    scope<renderer> renderer_ptr = nullptr;
    scope<driver_ui> driver_ui_ptr = nullptr;
    glm::vec2 viewport_size = { 0.0f, 0.0f };

    void handle_viewport_resize_event(const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_RENDERING_SYSTEM_HPP