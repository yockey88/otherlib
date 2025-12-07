/**
 * \file editor_driver.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_DRIVER_HPP
#define OTHER_EDITOR_EDITOR_DRIVER_HPP

#include "core/state_machine.hpp"

#include "driver/driver.hpp"

#include "editor_state_machine.hpp"
#include "editor_ui.hpp"
// #include "scripting/scri"

namespace other {

  class editor_driver : public driver {
   public:
    OTHER_APPLICATION_DRIVER("Other Editor");

    editor_driver(const config_table& config)
        : driver(config) {}
    ~editor_driver() override {}

    void on_initialize(const command_line&) override;
    void run() override;
    void on_shutdown() override;

   private:
    friend class editor_state_machine;
    editor_state_machine state_machine;

    lua_script* editor_lua_script = nullptr;

    scope<renderer> renderer = nullptr;
    scope<editor_ui> ui_ptr = nullptr;

    void core_update();
    void update_initializing();
    void update_running();
    void update_shutting_down();

    void on_event(SDL_Event* event) override;

    void initialize_ui();
    void shutdown_ui();
  };

}  // namespace other

OTHER_DRIVER(other::editor_driver);

#endif  // OTHER_EDITOR_EDITOR_DRIVER_HPP