/**
 * @file thread/thread.hpp
 */
#ifndef OTHER_CORE_THREAD_THREAD_HPP
#define OTHER_CORE_THREAD_THREAD_HPP

#include <chrono>
#include <thread>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "thread/channel.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

namespace other {

  class thread {
   public:
    enum state {
      WAITING = 0,
      LAUNCHING,

      STARTED,
      PROCESSING,

      SHUTTING_DOWN,
      STOPPED,
    };

    thread(const std::string& thread_name)
        : thread_name(thread_name) {}
    virtual ~thread() = default;

    inline bool is_running() {
      return current_state == STARTED ||
        current_state == PROCESSING ||
        current_state == WAITING;
    }

    void launch();
    void shutdown();
    void force_shutdown();

    opt<message> receive_message(microseconds timeout = microseconds(100));
    void send_message(message&& msg);

    state get_current_state();

    inline decltype(auto) aquire() {
      return std::lock_guard(thread_state_mutex);
    }

    struct threadlocal_data {
      scope<message_channel> tx_channel;
      scope<message_channel> rx_channel;
      std::stop_token stoken;
    };

    virtual void on_initialize() {}
    virtual void on_start() {}
    virtual void on_shutdown() {}
    virtual void pump_thread() {}
    virtual void handle_acknowledgement(const acknowledgement& ack) {}
    virtual void handle_ping(const session_status_request& ping) {}
    virtual void handle_pong(const session_status_response& pong) {}
    virtual void handle_shutdown_request(const session_shutdown_request& shutdown_request) {}

    std::string get_thread_name();

   protected:
    enum message_id {
      THREAD_INITIALIZE = 0,
      THREAD_START,
      THREAD_SHUTDOWN,
    };

    void set_current_state(state new_state);
    bool is_in_state(state check_state);

    void thread_send_message(message&& msg);

    /// add more here as needed

   private:
    const std::string thread_name;
    void wait_for_ack();

    struct state_flags {
      /// mixed used
      std::atomic<bool> running = false;
      std::atomic<bool> finalized = false;

      std::atomic<bool> error_occurred = false;

      std::atomic<bool> force_exit = false;

      /// thread used
      bool initialized = false;
    } checkpoints;

    std::mutex thread_state_mutex;
    std::jthread thread_handle;
    opt<integer_t> thread_exit_code = std::nullopt;

    state current_state = WAITING;
    scope<channel<message>> tx_channel;
    scope<channel<message>> rx_channel;

    void run(std::stop_token stoken, scope<channel<message>> thread_rx_channel, scope<channel<message>> thread_tx_channel);

    /// only ever called from thread where run() is executed
    void wait_for_initialization();
    void handle_init_msg(const message& msg);

    void wait_for_start();
    void handle_start_msg(const message& msg);

    void wait_for_shutdown();
    void handle_shutdown_msg(const message& msg);

    void handle_message(const message& msg);

    void handle_notification_message(const message& msg);
    void handle_acknowledgement_message(const message& msg);
    void handle_control_message(const message& msg);
    void handle_command_message(const message& msg);
    void handle_request_message(const message& msg);
    void handle_response_message(const message& msg);
    void handle_error_alert_message(const message& msg);

    virtual inline microseconds get_message_timeout() {
      return microseconds(100);
    }
  };

}  // namespace other

#endif  // OTHER_CORE_THREAD_THREAD_HPP