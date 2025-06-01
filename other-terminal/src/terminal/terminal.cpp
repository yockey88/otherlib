/**
 * \file terminal/terminal.cpp
 **/
#include "terminal/terminal.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/formatting.hpp"
#include "core/logger.hpp"
#include "core/registers.hpp"
#include "core/string_utils.hpp"
#include "core/timer.hpp"

#include "renderer/renderer_backend.hpp"
#include "renderer/ui/ui_helpers.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace {

    terminal* terminal_instance = nullptr;

    void handle_event(SDL_Event* event) {
      terminal_instance->on_key_down(event);
    }

    // void BindLuaTerminal(sol::state& lua_state, Terminal* terminal);

  }  // anonymous  namespace

  terminal::terminal() {
    std::ranges::fill(input_buffer, '\0');
  }

  void terminal::run(const command_line& cmdline, const config_table& config) {
    terminal_instance = this;

    term_window_id = SDL_GetWindowID(other::subsystem<other::renderer_backend>::get()->get_main_window());
    other::add_event_callback(handle_event);

    initialize(cmdline, config);

    other::scope<other::renderer> renderer = other::make_scope<other::renderer>();
    other::frame_rate_enforcer<60> fps_enforcer;
    while (is_running) {
      other::pump_events();
      update();

      renderer->begin_frame();
      renderer->begin_ui_frame();

      draw();

      renderer->end_ui_frame();
      renderer->end_frame();
      fps_enforcer.wait();
    }

    shutdown();
    renderer = nullptr;

    CORE_LOG_INFO("Terminal shutdown complete. Goodbye.");
  }

  void terminal::on_key_down(SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == term_window_id) {
      stop();
    }
  }

  void terminal::initialize(const command_line& cmdline, const config_table& config) {
    // control_thread = other::make_scope<terminal_thread>(this);
    // control_thread->launch();
    is_running = true;
  }

  using namespace std::chrono_literals;
  void terminal::update() {
    // if (auto opt = control_thread->receive_message(0us); opt.has_value()) {
    //   handle_received_thread_message(*opt);
    // }
  }

  namespace {

    int text_input_callback(ImGuiInputTextCallbackData* data) {
      terminal* term = terminal_instance;
      if (!term) {
        return 0;
      }

      /// complete request
      if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
        // std::string_view input(data->Buf);
        // if (input.empty()) {
        //   return 0;
        // }

        // /// find matching commands
        // std::vector<std::string> matches;
        // for (const auto& cmd : term->cmd_parser.get_available_commands()) {
        //   if (cmd.name.starts_with(input)) {
        //     matches.push_back(cmd.name);
        //   }
        // }

        // if (matches.size() == 1) {
        //   std::ranges::copy(matches[0], data->Buf);
        //   data->BufTextLen = matches[0].size();
        //   data->BufDirty = true;
        // } else if (matches.size() > 1) {
        //   std::string match_list = fmtstr("Possible matches: {}", fmtstr_list(matches, ", "));
        //   term->push_message({ terminal_filter::INFO_FILTER, match_list }, false);
        // } else {
        //   term->push_message({ terminal_filter::ERROR_FILTER, "No matches found" }, false);
        // }
        // return 0;
      }
      /// history navigation
      else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        // if (term->history_cursor.has_value()) {
        //   if (data->EventKey == ImGuiKey_UpArrow) {
        //     if (*term->history_cursor > 0) {
        //       --(*term->history_cursor);
        //     }
        //   } else if (data->EventKey == ImGuiKey_DownArrow) {
        //     if (*term->history_cursor < term->terminal_history.size() - 1) {
        //       ++(*term->history_cursor);
        //     }
        //   }

        //   const terminal_message& m = term->terminal_history[*term->history_cursor];
        //   std::ranges::fill(term->input_buffer, '\0');
        //   std::ranges::copy(m.message, term->input_buffer.begin());
        //   data->BufTextLen = m.message.size();
        //   data->BufDirty = true;
        // }
        // return 0;
      }

      return 0;
    }

  }  // namespace

  void terminal::draw() {
    bool open = true;
    if (ImGui::Begin("OtherEnvironment:Terminal", &open)) {
      const ImVec2 avail = ImGui::GetContentRegionAvail();
      const ImVec2 original_cursor_pos = ImGui::GetCursorPos();

      const size_t line_h = ImGui::GetTextLineHeight();
      const float line_padding = 8.f;
      const float full_line_h = line_h + line_padding;

      /// set the cursor at the bottom of the window (minus the height of the input box)
      ImVec2 text_input_pos = { original_cursor_pos.x, avail.y };
      ImGui::SetCursorPos(text_input_pos);

      /// input box
      {
        scoped_color text_bg_color(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
        ImVec2 size = { avail.x, static_cast<float>(line_h + 8.f) };

        // if (history_cursor.has_value()) {
        //   OTHER_ASSERT(*history_cursor < stored_history.size(), "History cursor out of bounds!");

        //   const terminal_message& m = stored_history[*history_cursor];
        //   std::ranges::fill(input_buffer, '\0');
        //   std::ranges::copy(m.message, input_buffer.begin());
        // }

        // clang-format off
        if (ImGui::InputTextEx("##console:input", "[ enter command ]", input_buffer.data(), input_buffer.size(),
                            size, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion,
                             &text_input_callback, this)) {
          handle_input();
        }
        // clang-format on
      }

      text_input_pos.x += 1.f;
      for (auto itr = terminal_history.rbegin(); itr != terminal_history.rend(); ++itr) {
        text_input_pos.y -= full_line_h;
        if (text_input_pos.y == original_cursor_pos.y) {
          break;
        }

        ImGui::SetCursorPos(text_input_pos);
        {
          glm::vec4 color = get_color_for_filter(itr->filters);
          ImVec4 text_color = ImVec4(color.r, color.g, color.b, color.a);
          scoped_color text_color_scope(ImGuiCol_Text, text_color);
          ImGui::Text("%s", itr->message.c_str());
        }
      }
    }
    ImGui::End();
  }

  void terminal::shutdown() {
    // control_thread->shutdown();
    // control_thread = nullptr;
  }

  void terminal::push_message(const terminal_message& message, bool save) {
    // message_buffer.push(message);
    // terminal_history.push_back(message);
    // if (save) {
    //   stored_history.push_back(message);
    // }
    history_cursor = std::nullopt;
  }

  void terminal::push_command(const terminal_message& command) {
    raw_command cmd = cmd_parser.parse(command.message);
    if (cmd.name == "invalid") {
      push_message({ terminal_filter::ERROR_FILTER, std::format("Command not recognized : '{}'", command.message) }, false);
      return;
    }

    /// compile command to bytecode
    auto [compiled_command, arg_addresses] = compiler.compile(cmd);
    if (compiled_command.opcode == op_code::INVALID_OP) {
      push_message({ terminal_filter::ERROR_FILTER, std::format("Failed to compile command : '{}'", command.message) }, false);
      return;
    }

    /// send command to the control thread
    other_command_msg cmd_msg;
    cmd_msg.cmd = compiled_command;
    cmd_msg.args = std::move(arg_addresses);

    message msg;
    msg.header = { cmd_msg.category, cmd_msg.id };
    msg.data = cmd_msg.build();

    // control_thread->send_message(std::move(msg));
  }

  glm::vec4 terminal::get_color_for_filter(terminal_filter filter) const {
    if (filter & terminal_filter::ERROR_FILTER) {
      return { 1.f, 0.f, 0.f, 1.f };
    } else if (filter & terminal_filter::WARNING_FILTER) {
      return { 1.f, 1.f, 0.f, 1.f };
    } else if (filter & terminal_filter::INFO_FILTER) {
      return { 0.f, 1.f, 0.f, 1.f };
    } else if (filter & terminal_filter::DEBUG_FILTER) {
      return { 0.f, 0.f, 1.f, 1.f };
    } else {
      return { 1.f, 1.f, 1.f, 1.f };
    }
  }

  void terminal::stop() {
    CORE_LOG_INFO("Terminal shutting down...");
    is_running = false;
  }

  void terminal::handle_input() {
    std::string input = input_buffer.data();
    input_buffer.fill('\0');

    trim_front_and_back(input);
    if (input.empty()) {
      return;
    }

    /// TODO: retrieve active filters
    terminal_message message{ terminal_filter::NO_FILTER, input };
    push_command(message);
  }

  void terminal::handle_received_thread_message(const message& msg) {
    switch (msg.get_category()) {
        /**
        NOTIFICATION = 0,
        CONTROL,

        COMMAND,
        QUERY,
        RESPONSE,

        ACKNOWLEDGEMENT,
        ERROR_ALERT,

        INFO,
         */
      case message_category::CONTROL:
        break;

      case message_category::COMMAND:
        break;

      case message_category::QUERY:
        break;

      case message_category::RESPONSE:
        break;

      case message_category::ACKNOWLEDGEMENT:
        break;

      case message_category::ERROR_ALERT: {
        CORE_LOG_ERROR("Terminal received error alert: {}", msg.get_id());
      } break;

      default:
        CORE_LOG_WARN("Terminal received unsupported message category: {}", msg.get_category());
        break;
    }
  }

  void terminal::handle_control_message(const message& msg) {
    switch (msg.get_id()) {
      case message_id::PING:
      case message_id::PONG:
        /// no-op for terminal
        break;

      case message_id::SESSION_SHUTDOWN_REQUEST: {
        /// this is sent to terminal if the command executor executes an 'exit' command
        stop();
      } break;

      default:
        CORE_LOG_WARN("Terminal received unsupported control message: {}", msg.get_id());
        break;
    }
  }

  namespace {

    // void BindLuaTerminal(sol::state& lua_state, Terminal* terminal) {
    //   lua_state.new_enum(
    //     "TerminalFilter",
    //     "ERROR_FILTER", TerminalFilter::ERROR_FILTER,
    //     "WARNING_FILTER", TerminalFilter::WARNING_FILTER,
    //     "INFO_FILTER", TerminalFilter::INFO_FILTER,
    //     "DEBUG_FILTER", TerminalFilter::DEBUG_FILTER,
    //     "TRACE_FILTER", TerminalFilter::TRACE_FILTER,
    //     "BAD_FILTER", TerminalFilter::BAD_FILTER,
    //     "GOOD_FILTER", TerminalFilter::GOOD_FILTER,
    //     "TERMINAL_FILTER_ALL", TerminalFilter::TERMINAL_FILTER_ALL,
    //     "NUM_TERMINAL_FILTERS", TerminalFilter::NUM_TERMINAL_FILTERS
    //   );

    //   lua_state.new_usertype<TerminalMessage>(
    //     "TerminalMessage",
    //     "filters", &TerminalMessage::filters,
    //     "message", &TerminalMessage::message
    //   );

    //   lua_state.new_usertype<Terminal>(
    //     "Terminal",
    //     "push_message", [&](const std::string& message, TerminalFilter filter) { terminal->PushMessage({ filter, message }); },
    //     "push_command", &Terminal::PushCommand,
    //     "source_file", &Terminal::SourceFile,
    //     "clear", &Terminal::Clear
    //   );

    //   lua_state.set("terminal", terminal);

    //   // lua_state.new_usertype<CommandBlock>(
    //   //   "CommandBlock",
    //   //   "block_name", &CommandBlock::block_name,
    //   //   "command_queue", &CommandBlock::command_queue,
    //   //   "argument_queue", &CommandBlock::argument_queue
    //   // );

    //   // lua_state.new_usertype<Command>(
    //   //   "Command",
    //   //   "raw_data", &Command::data,
    //   //   "opcode", [](const Command& cmd) { return static_cast<OpCode>(cmd.opcode); },
    //   //   "num_args", [](const Command& cmd) { return cmd.num_args; }
    //   // );
    // }

  }  // anonymous namespace

}  // namespace other