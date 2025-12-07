/**
 * \file tools/environment_console.hpp
 **/
#ifndef OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP
#define OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP

#include <mutex>

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

#include "core/logger.hpp"
#include "core/timer.hpp"


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

    static void push_message(const console_input& input);
    static void submit_console_text(const std::string_view text, console_message_type type, system_timepoint time_point);

    static inline const std::vector<console_input>& get_console_history() { return history_lines; }

    static inline char* get_input_buffer() { return input_buffer.data(); }
    static inline void clear_input_buffer() { std::ranges::fill(input_buffer.begin(), input_buffer.end(), 0); }

    constexpr static inline size_t kInputBufferSize = 256;

   private:
    static std::vector<console_input> history_lines;
    static size_t max_history_lines;
    static lua_script* console_lua_script;

    static std::array<char, kInputBufferSize> input_buffer;
  };

  template <typename Mutex>
  class console_sink : public spdlog::sinks::base_sink<Mutex> {
   public:
    ~console_sink() override = default;

   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
      spdlog::memory_buf_t formatted;
      spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
      std::string output_msg = std::string{ formatted };

      console_message_type message_type = CONSOLE_MESSAGE_NONE;

      switch (msg.level) {
        case spdlog::level::trace: message_type = CONSOLE_MESSAGE_TRACE; break;
        case spdlog::level::debug: message_type = CONSOLE_MESSAGE_DEBUG; break;
        case spdlog::level::info: message_type = CONSOLE_MESSAGE_INFO; break;
        case spdlog::level::warn: message_type = CONSOLE_MESSAGE_WARN; break;

        case spdlog::level::err:
        case spdlog::level::critical: message_type = CONSOLE_MESSAGE_ERROR; break;
        default:
          OTHER_ASSERT(false, "Unhandled log level in console sink: {}", msg.level);
          break;
      }

      environment_console::submit_console_text(output_msg, message_type, sys_clock::now());
    }

    void flush_() override {
    }
  };

  using console_sink_mt = console_sink<std::mutex>;
  using console_sink_st = console_sink<spdlog::details::null_mutex>;

}  // namespace other

#endif  // OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_HPP