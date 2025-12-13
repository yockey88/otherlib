/**
 * \file editor_driver.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_DRIVER_HPP
#define OTHER_EDITOR_EDITOR_DRIVER_HPP

#include "driver/driver.hpp"

#include "editor_ui.hpp"

namespace other {

  class OTHER_CLASS editor_driver : public driver {
   public:
    OTHER_APPLICATION_DRIVER("Other Editor");

    editor_driver(const config_table& config)
        : driver(config) {}
    ~editor_driver() override {}

    void on_initialize(const command_line&) override;
    void on_initialize_rendering(scope<renderer>& renderer_ptr) override;
    void on_update() override;
    void on_ui_render() override;
    void on_shutdown() override;
    void on_shutdown_rendering() override;

   private:
    lua_script* editor_lua_script = nullptr;

    scope<editor_ui> ui_ptr = nullptr;

    void update_running() override;

    void initialize_ui();
    void shutdown_ui();
  };

}  // namespace other

OTHER_DRIVER(other::editor_driver);

#endif  // OTHER_EDITOR_EDITOR_DRIVER_HPP