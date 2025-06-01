/**
 * \file terminal/terminal.cpp
 **/
#include "terminal.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "driver/driver.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/ui/ui_helpers.hpp"

// #include "event/event_queue.hpp"
// #include "memory/registers.hpp"

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

    // command_block.command_queue = {};
    // command_block.block_name = "Terminal";

    // lua_state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math, sol::lib::os);
    // BindLuaTerminal(lua_state, this);

    // EventQueue::RegisterEventDispatcher<KeyPressed>(
    //   "OtherEnv--CommandSubmitted",
    //   {
    //     std::bind_front(&Terminal::HandleEnterKey, this),
    //     std::bind_front(&Terminal::HandleUpDownKey, this),
    //   }
    // );
  }

  terminal::~terminal() {
    // Clear();
  }

  void terminal::run(const command_line& cmdline, const config_table& config) {
    terminal_instance = this;

    term_window_id = SDL_GetWindowID(other::subsystem<other::renderer_backend>::get()->get_main_window());
    other::add_event_callback(handle_event);

    initialize(cmdline, config);
    other::scope<other::renderer> renderer = other::make_scope<other::renderer>();

    is_running = true;
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
  }

  void terminal::on_key_down(SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
      if (event->window.windowID == term_window_id) {
        is_running = false;
        return;
      }
    }
    /// otherwise only handle key events for right nows
    else if (event->type != SDL_EVENT_KEY_DOWN) {
      return;
    }

    switch (event->key.key) {
      case SDLK_RETURN: {
        // std::string input = input_buffer.data();
        // input_buffer.fill('\0');

        // input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        // input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), input.end());
        // if (input.empty()) {
        //   return;
        // }

        // /// parse commmand
        // CORE_LOG_DEBUG("Command entered: {}", input);

        // history_cursor = std::nullopt;
      } break;

      case SDLK_BACKSPACE: {
        if (!input_buffer.empty()) {
          /// input_buffer[--input_cursor] = '\0';
        }
      } break;

      default: break;
    }

    // KeyPressed key_event(event->key);
    // if (HandleEnterKey(key_event)) {
    //   return;
    // }
    // if (HandleUpDownKey(key_event)) {
    //   return;
    // }

    // if (key_event.Key() == Keyboard::Key::OE_BACKSPACE) {
    //   input_buffer.pop_back();
    // } else if (key_event.Key() == Keyboard::Key::OE_RETURN) {
    //   std::string command(input_buffer.data());
    //   input_buffer.fill('\0');
    //   push_command({ TerminalFilter::COMMAND_FILTER, command });
    // } else {
    //   input_buffer.push_back(key_event.KeyChar());
    // }
  }

  void terminal::initialize(const command_line& cmdline, const config_table& config) {
    /// launch control thread
  }

  void terminal::update() {
  }

  namespace {

    int text_input_callback(ImGuiInputTextCallbackData* data) {
      terminal* term = terminal_instance;
      if (!term) {
        return 0;  // No terminal instance available
      }

      return 0;  // Accept the character
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
          std::string input = input_buffer.data();
          input_buffer.fill('\0');
          push_command({ terminal_filter::NO_FILTER, input });
        }
        // clang-format on
      }

      // text_input_pos.x += 1.f;
      // for (auto itr = terminal_history.rbegin(); itr != terminal_history.rend(); ++itr) {
      //   text_input_pos.y -= full_line_h;
      //   if (text_input_pos.y == original_cursor_pos.y) {
      //     break;
      //   }

      //   ImGui::SetCursorPos(text_input_pos);
      //   {
      //     glm::vec4 color = get_color_for_filter(itr->filters);
      //     ImVec4 text_color = ImVec4(color.r, color.g, color.b, color.a);
      //     scoped_color text_color_scope(ImGuiCol_Text, text_color);
      //     ImGui::Text("%s", itr->message.c_str());
      //   }
      // }
    }
    ImGui::End();
  }

  void terminal::shutdown() {
  }

  void terminal::push_message(const terminal_message& message, bool save) {
    message_buffer.push(message);
    terminal_history.push_back(message);
    if (save) {
      stored_history.push_back(message);
    }
    history_cursor = std::nullopt;
  }

  void terminal::push_command(const terminal_message& command) {
    // RawCommand raw_command = parser.Parse(command.message);
    // if (raw_command.name == "invalid") {
    //   PushMessage({ TerminalFilter::ERROR_FILTER, fmtstr("Command not recognized : '{}'", command.message) }, false);
    //   return;
    // }

    // /// compile command to bytecode
    // auto [compiled_command, arg_addresses] = compiler.Compile(raw_command);
    // if (compiled_command.opcode == OpCode::INVALID_OP) {
    //   return;
    // }

    /// queue command for dispatch
    // terminal_history.push_back(command);
    // stored_history.push_back(command);
    // command_block.command_queue.push(compiled_command);
    // for (const auto& addr : arg_addresses) {
    //   command_block.argument_queue.push(addr);
    // }
    history_cursor = std::nullopt;
  }

  // void Terminal::SourceFile(const Path& file_path) {
  //   switch (FNV(file_path.extension().string())) {
  //     case FNV(".ocmd"):
  //       SourceCmdFile(file_path);
  //       break;
  //     case FNV(".lua"):
  //       SourceLuaFile(file_path);
  //       break;
  //     case FNV(".py"):
  //       SourcePythonFile(file_path);
  //       break;

  //     default:
  //       OE_ERROR("Unrecognized file extension : {}", file_path.extension());
  //       PushMessage({ TerminalFilter::ERROR_FILTER, fmtstr("Unrecognized file extension : '{}'", file_path.extension()) });
  //       break;
  //   }
  // }

  // void Terminal::Clear() {
  //   terminal_history.clear();
  //   message_buffer = {};
  // }

  // void Terminal::Dispatch() {
  //   if (command_block.command_queue.empty()) {
  //     return;
  //   }

  //   ExitCode ec = executor.Execute(command_block);
  //   if (ec != ExitCode::SUCCESS) {
  //     OE_ERROR("Terminal executor failed : {}", ec);
  //     PushMessage({ TerminalFilter::ERROR_FILTER, "Terminal executor failed" });
  //   }
  //   OE_ASSERT(command_block.command_queue.empty(), "Command queue not empty after dispatch!");
  // }

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

  // void Terminal::SourceCmdFile(const Path& file_path) {
  //   std::vector<RawCommand> raw_commands = parser.ParseFile(file_path);
  //   if (raw_commands.empty()) {
  //     OE_ERROR("Failed to parse file : {}", file_path.string());
  //     PushMessage({ TerminalFilter::ERROR_FILTER, fmtstr("Failed to parse file : '{}'", file_path.string()) });
  //     return;
  //   }

  //   std::vector<CommandData> compiled_commands = compiler.Compile(raw_commands);
  //   if (compiled_commands.empty()) {
  //     OE_ERROR("Failed to compile commands from file : {}", file_path.string());
  //     PushMessage({ TerminalFilter::ERROR_FILTER, fmtstr("Failed to compile commands from file : '{}'", file_path.string()) });
  //     return;
  //   }

  //   for (const auto& [command, arg_addresses] : compiled_commands) {
  //     command_block.command_queue.push(command);
  //     for (const auto& addr : arg_addresses) {
  //       command_block.argument_queue.push(addr);
  //     }
  //   }
  // }

  // void Terminal::SourceLuaFile(const Path& file_path) {
  //   lua_state.script_file(file_path.string());
  // }

  // void Terminal::SourcePythonFile(const Path& file_path) {
  //   PushMessage({ TerminalFilter::ERROR_FILTER, "Python scripting not yet supported" });
  // }

  // bool Terminal::HandleEnterKey(KeyPressed& event) {
  //   if (event.Key() != Keyboard::Key::OE_RETURN) {
  //     return false;
  //   }

  //   std::string input = input_buffer.data();
  //   input_buffer.fill('\0');

  //   input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char ch) {
  //                 return !std::isspace(ch);
  //               }));
  //   input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char ch) {
  //                 return !std::isspace(ch);
  //               }).base(),
  //               input.end());
  //   if (input.empty()) {
  //     return false;
  //   }

  //   /// TODO: retrieve active filters
  //   TerminalMessage message{ TerminalFilter::NO_FILTER, input };
  //   PushCommand(message);
  //   return false;
  // }

  // bool Terminal::HandleUpDownKey(KeyPressed& event) {
  //   if (event.Key() != Keyboard::Key::OE_UP && event.Key() != Keyboard::Key::OE_DOWN) {
  //     return false;
  //   }

  //   if (stored_history.empty()) {
  //     return false;
  //   }

  //   if (!history_cursor.has_value()) {
  //     history_cursor = stored_history.size() - 1;
  //   } else {
  //     if (event.Key() == Keyboard::Key::OE_UP) {
  //       if (history_cursor.value() > 0) {
  //         --history_cursor.value();
  //       }
  //     } else {
  //       if (history_cursor.value() < stored_history.size() - 1) {
  //         ++history_cursor.value();
  //       }
  //     }
  //   }

  //   return false;
  // }

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