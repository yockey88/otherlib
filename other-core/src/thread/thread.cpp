/**
 * \file thread/thread.cpp
 **/
#include "thread/thread.hpp"

#include <atomic>

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "message/message_fields.hpp"

namespace other {

  static thread_local thread::threadlocal_data* thread_data;

  void thread::launch() {
    PROFILE_SECTION("thread::launch");
    thread_handle = std::jthread([this](std::stop_token stoken) {
      auto [thread_rx_channel, thread_tx_channel] = thread_launch_setup();
      run(stoken, std::move(thread_rx_channel), std::move(thread_tx_channel));
    });

    // first sync, allows communication channels to be set up
    thread_sync_barrier.arrive_and_wait();
    OTHER_ASSERT(get_current_state() == LAUNCHING, "Thread [{}] failed to enter launching state after barrier synchronization.", thread_name);

    OTHER_ASSERT(tx_channel != nullptr, "Thread tx channel is null");
    OTHER_ASSERT(rx_channel != nullptr, "Thread rx channel is null");

    message init_msg(CONTROL, THREAD_INITIALIZE);
    message start_msg(CONTROL, THREAD_START);

    send_to_thread(std::move(init_msg));
    send_to_thread(std::move(start_msg));

    std::this_thread::yield();
    std::this_thread::yield();

    opt<message> msg_opt = receive_from_thread(microseconds(500));
    OTHER_ASSERT(msg_opt.has_value(), "Thread [{}] did not acknowledge start message", thread_name);
    // clang-format off
    OTHER_ASSERT((message_header{ msg_opt->category, msg_opt->id } == message_header{ ACKNOWLEDGEMENT, ACK }), 
                 "Thread [{}] sent invalid acknowledgment for start message: {}, expected ACKNOWLEDGEMENT.ACK",
                 thread_name, message_header{ msg_opt->category, msg_opt->id });
    // clang-format on

    message_header mh = *reinterpret_cast<const message_header*>(msg_opt->data.data());
    OTHER_ASSERT((mh == message_header{ CONTROL, THREAD_START }), "Received ACK with unexpected original message header: {}, expected CONTROL.THREAD_START", mh);
    CORE_LOG_TRACE("[MAIN-THREAD RX: {}.{}]", message_category{ msg_opt->category }, thread::message_id{ msg_opt->id });
  }

  void thread::shutdown() {
    PROFILE_SECTION("thread::shutdown");
    if (checkpoints.force_exit.load(std::memory_order_acquire)) {
      CORE_LOG_WARN("Thread [{}] is already in force exit mode, shutdown request ignored", thread_name);
      return;
    } else if (checkpoints.error_occurred.load(std::memory_order_acquire)) {
      CORE_LOG_WARN("Thread [{}] is in error state, shutdown request ignored", thread_name);
      return;
    } else if (get_current_state() == SHUTTING_DOWN || get_current_state() == STOPPED) {
      CORE_LOG_WARN("Thread [{}] is already shutting down or stopped, shutdown request ignored", thread_name);
      return;
    } else if (get_current_state() == LAUNCHING) {
      CORE_LOG_WARN("Thread [{}] is still launching, shutdown request ignored", thread_name);
      return;
    } else if (get_current_state() == CRASHED) {
      CORE_LOG_WARN("Thread [{}] is in crashed state, shutdown request ignored", thread_name);
      return;
    } else if (!thread_handle.joinable()) {
      CORE_LOG_WARN("Thread [{}] is not joinable, shutdown request ignored", thread_name);
      return;
    }

    CORE_LOG_DEBUG("Thread [{}] shutdown initiated", thread_name);
    thread_handle.request_stop();
    thread_sync_barrier.arrive_and_wait();

    message shutdown_msg(CONTROL, THREAD_SHUTDOWN);
    send_to_thread(std::move(shutdown_msg));
  }

  void thread::force_shutdown() {
    PROFILE_SECTION("thread::force_shutdown");
    if (checkpoints.force_exit.load(std::memory_order_acquire)) {
      CORE_LOG_WARN("Thread [{}] is already in force exit mode, force shutdown request ignored", thread_name);
      return;
    }
    checkpoints.force_exit.store(true, std::memory_order_release);

    CORE_LOG_DEBUG("Thread [{}] starting forced shutdown", thread_name);
    if (thread_handle.joinable()) {
      thread_handle.request_stop();
      thread_handle.join();
    }
  }

  void thread::wait_for_shutdown_complete() {
    PROFILE_SECTION("thread::wait_for_shutdown_complete");
    while (get_current_state() != STOPPED) {
      std::this_thread::yield();
    }

    opt<message> shutdown_complete_msg = receive_from_thread(get_message_timeout());
    if (!shutdown_complete_msg.has_value()) {
      // check if we have to force shutdown and
      /// \todo handle error states better
      if (checkpoints.error_occurred.load(std::memory_order_acquire)) {
        CORE_LOG_ERROR("Thread [{}] is in error state, shutdown may not have completed cleanly", thread_name);
      } else {
        CORE_LOG_ERROR("Thread [{}] did not acknowledge shutdown completion, shutdown may not have completed cleanly", thread_name);
      }

      if (thread_handle.joinable()) {
        force_shutdown();
      }
      return;
    }
    OTHER_ASSERT((message_header{ shutdown_complete_msg->category, shutdown_complete_msg->id } == message_header{ ACKNOWLEDGEMENT, ACK }), "Thread [{}] sent invalid acknowledgment for shutdown complete message: {}, expected ACKNOWLEDGEMENT.ACK", thread_name, message_header{ shutdown_complete_msg->category, shutdown_complete_msg->id });
    CORE_LOG_TRACE("[MAIN-THREAD RX: {}.{}]", message_category{ shutdown_complete_msg->category }, thread::message_id{ shutdown_complete_msg->id });
    message_header mh = *reinterpret_cast<const message_header*>(shutdown_complete_msg->data.data());
    OTHER_ASSERT((mh == message_header{ CONTROL, THREAD_SHUTDOWN }), "Received ACK with unexpected original message header: {}, expected CONTROL.THREAD_SHUTDOWN", mh);
    CORE_LOG_DEBUG("Thread [{}] shutdown complete", thread_name);
  }

  opt<message> thread::receive_message(microseconds timeout) {
    if (timeout.count() == 0 && rx_channel->empty()) {
      return std::nullopt;
    }

    return rx_channel->await_message(timeout);
  }

  void thread::send_message(message&& msg) {
    OTHER_ASSERT(tx_channel != nullptr, "Thread tx channel is null");
    tx_channel->push(std::move(msg));
  }

  thread::state thread::get_current_state() {
    std::lock_guard lock(thread_state_mutex);
    return current_state;
  }

  void thread::set_current_state(state new_state) {
    std::lock_guard lock(thread_state_mutex);
    current_state = new_state;
  }

  bool thread::is_in_state(state check_state) {
    std::lock_guard lock(thread_state_mutex);
    return current_state == check_state;
  }

  std::string thread::get_thread_name() {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    std::lock_guard lock(thread_state_mutex);
    return thread_name;
  }

  void thread::handle_thread_error(const std::string_view error_message) {
    CORE_LOG_ERROR("Thread [{}] encountered error: {}", get_thread_name(), error_message);
    {
      std::lock_guard lock(thread_state_mutex);
      this->error_message = std::string{ error_message };
      thread_exit_code = -1;
    }

    checkpoints.error_occurred.store(true, std::memory_order_release);
    set_current_state(CRASHED);
  }

  void thread::send_to_thread(message&& msg) {
    OTHER_ASSERT(tx_channel != nullptr, "Thread tx channel is null");
    CORE_LOG_TRACE("[MAIN-THREAD TX: {}.{}]", message_category{ msg.category }, thread::message_id{ msg.id });
    tx_channel->push(std::move(msg));
  }

  void thread::send_to_main_thread(message&& msg) {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    OTHER_ASSERT(thread_data->rx_channel != nullptr, "Thread rx channel is null");
    CORE_LOG_TRACE("[THREAD TX: {}.{}]", message_category{ msg.category }, thread::message_id{ msg.id });
    thread_data->tx_channel->push(std::move(msg));
  }

  opt<message> thread::receive_from_thread(microseconds timeout) {
    OTHER_ASSERT(rx_channel != nullptr, "Thread rx channel is null");
    return rx_channel->await_message(timeout);
  }

  opt<message> thread::receive_from_main_thread(microseconds timeout) {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    OTHER_ASSERT(thread_data->rx_channel != nullptr, "Thread rx channel is null");
    return thread_data->rx_channel->await_message(timeout);
  }

  std::pair<scope<message_channel>, scope<message_channel>> thread::thread_launch_setup() {
    PROFILE_SECTION("thread::thread_launch_setup");
    ref<channel_queue<message>> tx_queue = make_ref<channel_queue<message>>();
    ref<channel_queue<message>> rx_queue = make_ref<channel_queue<message>>();

    auto [this_tx_channel, thread_rx_channel] = message_channel::make_channel(tx_queue);
    auto [thread_tx_channel, this_rx_channel] = message_channel::make_channel(rx_queue);

    {
      std::lock_guard lock(thread_state_mutex);
      tx_channel = std::move(this_tx_channel);
      rx_channel = std::move(this_rx_channel);
    }

    return { std::move(thread_rx_channel), std::move(thread_tx_channel) };
  }

  bool thread::thread_control_loop(std::stop_token& stoken) {
    do {
      PROFILE_SECTION("thread::thread_control_loop--iteration");
      try {
        opt<message> msg = receive_from_main_thread(get_message_timeout());
        if (!msg.has_value()) {
          if (checkpoints.error_occurred.load(std::memory_order_acquire) ||
              checkpoints.force_exit.load(std::memory_order_acquire) ||
              stoken.stop_requested()) {
            break;
          }
          continue;
        }
        CORE_LOG_TRACE("[THREAD CTRL RX: {}.{}]", message_category{ msg->category }, thread::message_id{ msg->id });

        if (msg->category != CONTROL) {
          CORE_LOG_ERROR("Unexpected message category received during thread control loop: {}, expected CONTROL", msg->category);
          checkpoints.error_occurred.store(true, std::memory_order_release);
          continue;
        }
        if (msg->id != THREAD_INITIALIZE && msg->id != THREAD_START && msg->id != THREAD_SHUTDOWN) {
          CORE_LOG_ERROR("Unexpected message ID received during thread control loop: {}, expected THREAD_INITIALIZE, THREAD_START or THREAD_SHUTDOWN", msg->id);
          checkpoints.error_occurred.store(true, std::memory_order_release);
          continue;
        }

        if (message_header{ msg->category, msg->id } == message_header{ CONTROL, THREAD_INITIALIZE }) {
          OTHER_ASSERT(!checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received initialization message but is already initialized.", thread_name);
          checkpoints.initialized.store(true, std::memory_order_release);
          on_initialize();
          CORE_LOG_DEBUG("Thread [{}] initialized", thread_name);
        } else if (message_header{ msg->category, msg->id } == message_header{ CONTROL, THREAD_START }) {
          OTHER_ASSERT(checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received start message but is not initialized.", thread_name);
          OTHER_ASSERT(!checkpoints.running.load(std::memory_order_acquire), "Thread [{}] received start message but is already running.", thread_name);
          checkpoints.running.store(true, std::memory_order_release);
          on_start();
          CORE_LOG_DEBUG("Thread [{}] started", thread_name);
        } else if (message_header{ msg->category, msg->id } == message_header{ CONTROL, THREAD_SHUTDOWN }) {
          OTHER_ASSERT(checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received shutdown message but is not initialized.", thread_name);
          OTHER_ASSERT(checkpoints.running.load(std::memory_order_acquire), "Thread [{}] received shutdown message but is not running.", thread_name);
          checkpoints.initialized.store(false, std::memory_order_release);
          checkpoints.running.store(false, std::memory_order_release);
          checkpoints.error_occurred.store(false, std::memory_order_release);
          on_shutdown();
          CORE_LOG_DEBUG("Thread [{}] shutdown confirmed", thread_name);
        } else {
          OTHER_ASSERT(false, "Unreachable code reached in thread control loop");
        }

        // on start we want init, start and on shutdown we just want shutdown
        if (message_header{ msg->category, msg->id } == message_header{ CONTROL, THREAD_START } ||
            message_header{ msg->category, msg->id } == message_header{ CONTROL, THREAD_SHUTDOWN }) {
          message ack(ACKNOWLEDGEMENT, ACK);
          message_header original_header = { msg->category, msg->id };
          const uint8_t* ack_data = reinterpret_cast<const uint8_t*>(&original_header);
          ack.data.append_range(std::span(ack_data, sizeof(message_header)));
          send_to_main_thread(std::move(ack));
          return true;
        }

      } catch (const std::exception& e) {
        handle_thread_error(std::format("Exception during thread control loop: {}", e.what()));
      } catch (...) {
        handle_thread_error("Unknown exception during thread control loop");
      }
    } while (!stoken.stop_requested());

    if (checkpoints.error_occurred.load(std::memory_order_acquire)) {
      // exception logger error already
    } else if (checkpoints.force_exit.load(std::memory_order_acquire)) {
      CORE_LOG_WARN("Thread [{}] exiting due to force exit flag during initialization or start", get_thread_name());
    } else if (stoken.stop_requested()) {
      CORE_LOG_WARN("Thread [{}] stop requested during initialization or start", get_thread_name());
    }
    return false;
  }

  void thread::main_loop(std::stop_token& stoken) {
    do {
      PROFILE_SECTION("thread::main_loop--iteration");
      try {
        set_current_state(WAITING);
        opt<message> msg = receive_from_main_thread(get_message_timeout());
        set_current_state(PROCESSING);
        if (msg) {
          handle_message(*msg);
        }
        /// no pending message
        else {
          pump_thread();
        }
      } catch (const std::runtime_error& e) {
        handle_thread_error(std::format("Runtime error: {}", e.what()));
      } catch (const std::exception& e) {
        handle_thread_error(std::format("Exception: {}", e.what()));
      } catch (...) {
        handle_thread_error("Unknown exception occurred");
      }
    } while (checkpoints.running.load(std::memory_order_acquire) && thread_loop_condition(stoken));
  }

  void thread::run(std::stop_token stoken, scope<message_channel> thread_rx_channel, scope<message_channel> thread_tx_channel) {
    threadlocal_data threadlocal_data;
    threadlocal_data.tx_channel = std::move(thread_tx_channel);
    threadlocal_data.rx_channel = std::move(thread_rx_channel);
    threadlocal_data.stoken = stoken;
    thread_data = &threadlocal_data;

    set_current_state(LAUNCHING);
    // first sync, allows main thread to know it can send control messages for initialization
    thread_sync_barrier.arrive_and_wait();
    if (!thread_control_loop(stoken)) {
      set_current_state(STOPPED);
      return;
    }

    do {
      main_loop(stoken);

      bool continue_running = false;
      if (get_current_state() == CRASHED) {
        continue_running = on_thread_crash(error_message);
        if (continue_running) {
          CORE_LOG_WARN("Thread [{}] is recovering from crash and will continue running", get_thread_name());
        } else {
          CORE_LOG_ERROR("Thread [{}] has died", get_thread_name());
          checkpoints.running.store(false, std::memory_order_release);
        }
      }
    } while (checkpoints.running.load(std::memory_order_acquire) && thread_loop_condition(stoken));

    set_current_state(SHUTTING_DOWN);
    thread_sync_barrier.arrive_and_wait();

    if (checkpoints.force_exit.load(std::memory_order_acquire)) {
      CORE_LOG_WARN("Thread [{}] exiting due to force exit flag during processing loop", get_thread_name());
    } else {
      CORE_LOG_DEBUG("Thread [{}] exiting normally, confirming shutdown...", get_thread_name());
      thread_control_loop(stoken);
    }
    set_current_state(STOPPED);
  }

  void thread::handle_init_msg(const message& msg) {
    /// use raw header bc custom message type
    OTHER_ASSERT(!checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received initialization message but is already initialized.", thread_name);

    if (msg.category == CONTROL && msg.id == THREAD_INITIALIZE) {
      checkpoints.initialized.store(true, std::memory_order_release);
      checkpoints.error_occurred.store(false, std::memory_order_release);
      on_initialize();
      CORE_LOG_DEBUG("Thread [{}] initialized", get_thread_name());
    } else {
      CORE_LOG_ERROR("Invalid message type for thread initialization : {}:{}", msg.get_category(), msg.get_id());
      checkpoints.error_occurred.store(true, std::memory_order_release);
      thread_exit_code = -1;
    }
  }

  void thread::handle_start_msg(const message& msg) {
    OTHER_ASSERT(checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received start message but is not initialized.", thread_name);
    OTHER_ASSERT(!checkpoints.running.load(std::memory_order_acquire), "Thread [{}] received start message but is already running.", thread_name);

    /// use raw header bc custom message type
    if (msg.category == CONTROL && msg.id == THREAD_START) {
      checkpoints.running.store(true, std::memory_order_release);
      checkpoints.error_occurred.store(false, std::memory_order_release);
      on_start();
      CORE_LOG_DEBUG("Thread [{}] started", get_thread_name());
    } else {
      CORE_LOG_ERROR("Invalid message type for thread start : {}:{}", msg.get_category(), msg.get_id());
      checkpoints.error_occurred.store(true, std::memory_order_release);
      thread_exit_code = -1;
    }
  }

  void thread::handle_shutdown_msg(const message& msg) {
    OTHER_ASSERT(checkpoints.initialized.load(std::memory_order_acquire), "Thread [{}] received shutdown message but is not initialized.", thread_name);
    OTHER_ASSERT(checkpoints.running.load(std::memory_order_acquire), "Thread [{}] received shutdown message but is not running or in error state.", thread_name);

    /// use raw header bc custom message type
    if (msg.id == THREAD_SHUTDOWN) {
      checkpoints.running.store(false, std::memory_order_release);
      checkpoints.error_occurred.store(false, std::memory_order_release);
      on_shutdown();
      CORE_LOG_DEBUG("Thread [{}] shut down", get_thread_name());
    } else {
      CORE_LOG_DEBUG("Invalid message type for thread shutdown : {}:{}", msg.get_category(), msg.get_id());
      checkpoints.error_occurred.store(true, std::memory_order_release);
      thread_exit_code = -1;
    }
  }

  void thread::handle_message(const message& msg) {
    PROFILE_SECTION("thread::handle_message");
    CORE_LOG_TRACE("[THREAD RX: {}]", message_header{ msg.category, msg.id });
    switch (msg.get_category()) {
      case message_category::NOTIFICATION: handle_notification_message(msg); break;
      case message_category::ACKNOWLEDGEMENT: handle_acknowledgement_message(msg); break;
      case message_category::CONTROL: handle_control_message(msg); break;
      case message_category::COMMAND: handle_command_message(msg); break;
      case message_category::REQUEST: handle_request_message(msg); break;
      case message_category::RESPONSE: handle_response_message(msg); break;
      case message_category::ERROR_ALERT: handle_error_alert_message(msg); break;
      default:
        CORE_LOG_WARN("Terminal received unsupported message category: {}", msg.get_category());
        break;
    }
  }

  void thread::handle_notification_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == NOTIFICATION, "Invalid notification message category: {}", message_header{ msg.category, msg.id });
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported notification message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_acknowledgement_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == ACKNOWLEDGEMENT, "Invalid acknowledgement message category: {}", message_header{ msg.category, msg.id });
  }

  void thread::handle_control_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == CONTROL, "Invalid control message category: {}", message_header{ msg.category, msg.id });
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported control message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_command_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == COMMAND, "Invalid command message category: {}", message_header{ msg.category, msg.id });
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported command message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_request_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == REQUEST, "Invalid request message category: {}", message_header{ msg.category, msg.id });
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported request message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_response_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == RESPONSE, "Invalid response message category: {}", message_header{ msg.category, msg.id });
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported response message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_error_alert_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == ERROR_ALERT, "Invalid error alert message category: {}", message_header{ msg.category, msg.id });
    CORE_LOG_ERROR("Thread received error alert: {}", msg.get_id());
    switch (msg.get_id()) {
      default:
        break;
    }
  }

}  // namespace other