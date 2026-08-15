/**
 * @file thread/thread.hpp
 */
#ifndef OTHER_CORE_THREAD_THREAD_HPP
#define OTHER_CORE_THREAD_THREAD_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "thread/channel.hpp"

#include "message/message.hpp"

namespace other {

  class OTHER_CLASS thread {
   public:
    enum state {
      WAITING = 0,
      LAUNCHING,

      PROCESSING,
      PUMPING,

      SHUTTING_DOWN,
      CRASHED,
      STOPPED,
    };

    thread(const std::string& thread_name)
        : thread_name(thread_name) {}
    virtual ~thread() = default;

    inline bool is_running() {
      const state snapshot = current_state.load(std::memory_order_acquire);
      return snapshot == WAITING ||
        snapshot == PROCESSING ||
        snapshot == PUMPING;
    }

    void launch();
    void shutdown();
    void force_shutdown();
    void wait_for_shutdown_complete();

    opt<message> receive_message(microseconds timeout = microseconds(100));
    void send_message(message&& msg);

    state get_current_state();

    inline decltype(auto) aquire() {
      return std::lock_guard(thread_state_mutex);
    }

    inline bool thread_running() {
      return checkpoints.initialized.load(std::memory_order_acquire) &&
        checkpoints.running.load(std::memory_order_acquire) &&
        get_current_state() != LAUNCHING &&
        get_current_state() != SHUTTING_DOWN &&
        get_current_state() != STOPPED;
    }

    inline bool in_error_state() {
      return checkpoints.error_occurred.load(std::memory_order_acquire);
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
    // return true to continue running, false to exit immediately
    virtual bool on_thread_crash(const std::string& error_msg) { return false; }

    std::string get_thread_name();

   protected:
    enum message_id : uint16_t {
      THREAD_INITIALIZE = 0,
      THREAD_START,
      THREAD_SHUTDOWN,
    };

    void set_current_state(state new_state);
    bool is_in_state(state check_state);

   private:
    const std::string thread_name;

    struct state_flags {
      std::atomic<bool> initialized = false;
      std::atomic<bool> running = false;

      std::atomic<bool> finalized = false;
      std::atomic<bool> error_occurred = false;
      std::atomic<bool> force_exit = false;
      std::atomic<bool> shutdown_initiated = false;
    } checkpoints;

    std::string error_message;

    /// single-use two-party rendezvous. unlike std::barrier every wait is bounded and a
    ///  party can arrive without waiting, so no exit path can park a thread forever
    struct sync_point {
      std::mutex mutex;
      std::condition_variable cv;
      uint32_t arrivals = 0;

      void arrive() {
        {
          std::lock_guard lock(mutex);
          ++arrivals;
        }
        cv.notify_all();
      }

      /// true if the peer arrived within the timeout
      bool arrive_and_wait(microseconds timeout) {
        std::unique_lock lock(mutex);
        ++arrivals;
        cv.notify_all();
        return cv.wait_for(lock, timeout, [this] { return arrivals >= 2; });
      }
    };

    sync_point launch_sync;
    sync_point shutdown_sync;

    std::mutex thread_state_mutex;
    std::jthread thread_handle;
    opt<integer_t> thread_exit_code = std::nullopt;

    /// atomic: read off-thread (is_running gates sink reclamation) against worker writes
    std::atomic<state> current_state = WAITING;
    scope<message_channel> tx_channel;
    scope<message_channel> rx_channel;

    inline bool thread_loop_condition(std::stop_token& stoken) {
      return !checkpoints.error_occurred.load(std::memory_order_acquire) &&
        !checkpoints.force_exit.load(std::memory_order_acquire) &&
        !stoken.stop_requested();
    }

    void handle_thread_error(const std::string_view error_message);

    void send_to_thread(message&& msg);
    void send_to_main_thread(message&& msg);

    opt<message> receive_from_thread(microseconds timeout = microseconds(100));
    opt<message> receive_from_main_thread(microseconds timeout = microseconds(100));

    std::pair<scope<message_channel>, scope<message_channel>> thread_launch_setup();
    // if false, immediately exit thread function, otherwise continue
    // this blocks thread
    bool thread_control_loop(std::stop_token& stoken);
    void main_loop(std::stop_token& stoken);
    void run(std::stop_token stoken, scope<message_channel> thread_rx_channel, scope<message_channel> thread_tx_channel);

    /// only ever called from thread where run() is executed
    void handle_init_msg(const message& msg);
    void handle_start_msg(const message& msg);
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