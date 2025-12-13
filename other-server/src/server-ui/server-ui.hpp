/**
 * \file server-ui/server-ui.hpp
 **/
#ifndef OTHER_SERVER_SERVER_UI_SERVER_UI_HPP
#define OTHER_SERVER_SERVER_UI_SERVER_UI_HPP

#include <nfd/nfd.h>
#include <nlohmann/json.hpp>

#include "core/state_machine.hpp"

#include "renderer/renderer.hpp"
#include "renderer/ui/ui_window.hpp"

namespace json = nlohmann;

namespace other {

  enum ui_state {
    UI_STATE_PROJECT_PAGE = 0,
    UI_STATE_CREATE_PROJECT_PAGE,
    UI_STATE_SETTINGS_PAGE,

    NUM_STATES,
  };

  enum ui_event {
    UI_EVENT_GO_TO_PROJECT_PAGE = 0,
    UI_EVENT_GO_TO_CREATE_PROJECT_PAGE,
    UI_EVENT_GO_TO_SETTINGS_PAGE,
    UI_EVENT_GO_TO_HOME_PAGE,  /// project page

    NUM_EVENTS,
  };

  class ui_state_machine : public state_machine<ui_state, ui_event> {
   public:
    ui_state_machine()
        : state_machine<ui_state, ui_event>(ui_state::UI_STATE_PROJECT_PAGE) {
      add_transition(ui_state::UI_STATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_CREATE_PROJECT_PAGE, ui_state::UI_STATE_CREATE_PROJECT_PAGE);
      add_transition(ui_state::UI_STATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_SETTINGS_PAGE, ui_state::UI_STATE_SETTINGS_PAGE);
      add_transition(ui_state::UI_STATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_HOME_PAGE, ui_state::UI_STATE_PROJECT_PAGE);

      add_transition(ui_state::UI_STATE_CREATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_PROJECT_PAGE, ui_state::UI_STATE_PROJECT_PAGE);
      add_transition(ui_state::UI_STATE_CREATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_SETTINGS_PAGE, ui_state::UI_STATE_SETTINGS_PAGE);
      add_transition(ui_state::UI_STATE_CREATE_PROJECT_PAGE, ui_event::UI_EVENT_GO_TO_HOME_PAGE, ui_state::UI_STATE_PROJECT_PAGE);

      add_transition(ui_state::UI_STATE_SETTINGS_PAGE, ui_event::UI_EVENT_GO_TO_PROJECT_PAGE, ui_state::UI_STATE_PROJECT_PAGE);
      add_transition(ui_state::UI_STATE_SETTINGS_PAGE, ui_event::UI_EVENT_GO_TO_CREATE_PROJECT_PAGE, ui_state::UI_STATE_CREATE_PROJECT_PAGE);
      add_transition(ui_state::UI_STATE_SETTINGS_PAGE, ui_event::UI_EVENT_GO_TO_HOME_PAGE, ui_state::UI_STATE_PROJECT_PAGE);
    }
    virtual ~ui_state_machine() = default;

    void on_enter_state(ui_state new_state) override {
      CORE_LOG_DEBUG("UI state changed to {}", new_state);
    }
  };

  class server_ui {
   public:
    server_ui(scope<event_system>& events, json::json& project_cache);
    ~server_ui() = default;

    void render();

   private:
    ui_state_machine state_machine;

    scope<event_system>& events;
    json::json& project_cache;

    scope<ui_window> project_win;
    scope<ui_window> create_project_win;

    void render_all();

    void render_all_project_page();
    void render_all_create_project_page();
    void render_all_settings_page();

    void validate_object_and_render_project(const json::json& json_obj);
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_UI_SERVER_UI_HPP