/**
 * \file tools/environment_console.hpp
 **/
#ifndef OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP
#define OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP

#include <mutex>
#include <queue>

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

#include "core/logger.hpp"
#include "core/scope.hpp"
#include "core/time.hpp"
#include "event/event_system.hpp"

namespace other {

  class lua_script;

  enum console_message_type : int32_t {
    CONSOLE_MESSAGE_NONE = 0,
    CONSOLE_MESSAGE_MESSAGE = 1 << 0,
    CONSOLE_MESSAGE_TRACE = 1 << 1,
    CONSOLE_MESSAGE_DEBUG = 1 << 2,
    CONSOLE_MESSAGE_INFO = 1 << 3,
    CONSOLE_MESSAGE_WARN = 1 << 4,
    CONSOLE_MESSAGE_ERROR = 1 << 5,
    CONSOLE_MESSAGE_COMMAND = 1 << 6,
  };
  struct console_input {
    std::string input_text = "";
    int32_t message_type = CONSOLE_MESSAGE_NONE;
    system_timepoint timestamp;
  };

  class environment_console {
   public:
    static void initialize(lua_script* console_script);
    static inline bool is_initialized() {
      return console_lua_script != nullptr && console_initialized;
    }

    static void poll();

    static void push_message(const console_input& input);
    static void submit_console_text(const std::string_view text, console_message_type type, system_timepoint time_point = sys_clock::now());

    static void clear_console_output();

    static inline const std::vector<console_input>& get_console_history() { return history_lines; }

    static inline char* get_input_buffer() { return input_buffer.data(); }
    static inline void clear_input_buffer() {
      std::ranges::fill(input_buffer.begin(), input_buffer.end(), 0);
    }
    static inline size_t get_cursor_position() { return history_cursor; }

    enum history_move {
      HISTORY_MOVE_NONE = 0,
      HISTORY_MOVE_BACK,
      HISTORY_MOVE_FORWARD
    };
    static void move_history_cursor(history_move move);

    constexpr static inline size_t kInputBufferSize = 1024 * 2;

   private:
    /// must always use console on thread that initialized it
    static std::thread::id console_thread_id;

    static std::atomic<bool> console_initialized;

    static std::mutex input_mutex;
    static std::atomic<bool> input_waiting;
    static std::queue<console_input> input_queue;

    static size_t history_cursor;
    static std::vector<console_input> history_lines;
    static std::vector<console_input> long_term_history_lines;

    static size_t max_history_lines;
    static lua_script* console_lua_script;

    static std::array<char, kInputBufferSize> input_buffer;
  };

  template <typename Mutex>
  class console_sink : public spdlog::sinks::base_sink<Mutex> {
   public:
    console_sink(scope<event_system>& events)
        : events(events) {}
    ~console_sink() override = default;

   protected:
    scope<event_system>& events;

    void sink_it_(const spdlog::details::log_msg& msg) override {
      if (!events) {
        return;
      }

      spdlog::memory_buf_t formatted;

      using namespace spdlog::sinks;
      base_sink<Mutex>::formatter_->format(msg, formatted);
      std::string output_msg = std::string{ formatted };
      switch (msg.level) {
        case spdlog::level::trace: events->trigger_event("console.trace", output_msg); break;
        case spdlog::level::debug: events->trigger_event("console.debug", output_msg); break;
        case spdlog::level::info: events->trigger_event("console.info", output_msg); break;
        case spdlog::level::warn: events->trigger_event("console.warn", output_msg); break;
        case spdlog::level::err: events->trigger_event("console.error", output_msg); break;
        case spdlog::level::critical: events->trigger_event("console.critical", output_msg); break;
        default:
          OTHER_ASSERT(false, "Unhandled log level in console sink: {}", msg.level);
          break;
      }
    }

    void flush_() override {
    }
  };

  using console_sink_mt = console_sink<std::mutex>;
  using console_sink_st = console_sink<spdlog::details::null_mutex>;

}  // namespace other

#endif  // OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP