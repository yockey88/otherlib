/**
 * \file thread/thread.cpp
 **/
#include "thread/thread.hpp"

#include "core/logger.hpp"
#include "thread/message.hpp"

namespace other {

  static thread_local thread::threadlocal_data* thread_data;

  void thread::launch() {
    set_current_state(WAITING);
    thread_handle = std::jthread([this](std::stop_token stoken) {
      ref<channel_queue<message>> tx_queue = make_ref<channel_queue<message>>();
      ref<channel_queue<message>> rx_queue = make_ref<channel_queue<message>>();

      auto [this_tx_channel, thread_rx_channel] = channel<message>::make_channel(tx_queue);
      auto [thread_tx_channel, this_rx_channel] = channel<message>::make_channel(rx_queue);

      {
        std::lock_guard lock(thread_state_mutex);
        tx_channel = std::move(this_tx_channel);
        rx_channel = std::move(this_rx_channel);
      }
      set_current_state(LAUNCHING);

      run(stoken, std::move(thread_rx_channel), std::move(thread_tx_channel));

      {
        std::lock_guard lock(thread_state_mutex);
        checkpoints.finalized = true;
      }
      set_current_state(STOPPED);
    });
    while (is_in_state(WAITING)) {
      std::this_thread::yield();
    }

    OTHER_ASSERT(tx_channel != nullptr, "Thread tx channel is null");
    OTHER_ASSERT(rx_channel != nullptr, "Thread rx channel is null");

    {
      message init_msg(CONTROL, THREAD_INITIALIZE);
      tx_channel->push(std::move(init_msg));
      wait_for_ack();
    }

    if (checkpoints.error_occurred) {
      CORE_LOG_ERROR("Thread launch error");
      return;
    }

    {
      message start_msg(CONTROL, THREAD_START);
      tx_channel->push(std::move(start_msg));
      /// no ack to start
    }

    if (checkpoints.error_occurred) {
      CORE_LOG_ERROR("Thread start error");
      return;
    }
    while (is_in_state(LAUNCHING)) {
      std::this_thread::yield();
    }
  }

  void thread::shutdown() {
    thread_handle.request_stop();

    while (!is_in_state(STOPPED)) {
      std::this_thread::yield();
    }
    CORE_LOG_DEBUG("Thread [{}] stopped", thread_name);
  }

  opt<message> thread::receive_message(std::chrono::microseconds timeout) {
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

  void thread::thread_send_message(message&& msg) {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    OTHER_ASSERT(thread_data->tx_channel != nullptr, "Thread tx channel is null");
    thread_data->tx_channel->push(std::move(msg));
  }

  void thread::wait_for_ack() {
    OTHER_ASSERT(tx_channel != nullptr, "Simulation thread tx channel is null");

    bool done = false;
    do {
      if (done) {
        break;
      }

      opt<message> msg = rx_channel->await_message(std::chrono::milliseconds(10));
      if (!msg) {
        continue;
      }

      if (msg->get_category() == ACKNOWLEDGEMENT) {
        if (msg->get_id() == ACK) {
          acknowledgement ack = acknowledgement::parse(msg->data);
          if (ack.ack_nack == 1) {
            return;
          } else {
            CORE_LOG_ERROR("Error occurred waiting for acknowledgement or start : {}:{}", msg->get_category(), msg->get_id());
            checkpoints.error_occurred = true;
            thread_exit_code = -1;
          }
        }
      } else if (msg->get_category() == ERROR_ALERT) {
        CORE_LOG_ERROR("Error alert received during thread initialization or start : {}:{}", msg->get_category(), msg->get_id());
        checkpoints.error_occurred = true;
        thread_exit_code = -1;
      }
    } while (!checkpoints.error_occurred);
  }

  void thread::run(std::stop_token stoken, scope<channel<message>> thread_rx_channel, scope<channel<message>> thread_tx_channel) {
    threadlocal_data threadlocal_data;
    threadlocal_data.tx_channel = std::move(thread_tx_channel);
    threadlocal_data.rx_channel = std::move(thread_rx_channel);
    threadlocal_data.stoken = stoken;
    thread_data = &threadlocal_data;

    wait_for_initialization();
    wait_for_start();

    do {
      try {
        set_current_state(WAITING);
        opt<message> msg = thread_data->rx_channel->await_message(std::chrono::milliseconds(100));
        set_current_state(PROCESSING);
        if (msg) {
          handle_message(*msg);
        }
        /// no pending message
        else {
          pump_thread();
        }
      } catch (const std::runtime_error& e) {
        CORE_LOG_ERROR("Runtime error in thread [{}] : {}", get_thread_name(), e.what());
        checkpoints.error_occurred = true;
        thread_exit_code = -1;
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception in thread [{}] : {}", get_thread_name(), e.what());
        checkpoints.error_occurred = true;
        thread_exit_code = -1;
      } catch (...) {
        CORE_LOG_ERROR("Unknown exception occurred in thread [{}]", get_thread_name());
      }
    } while (checkpoints.running && !stoken.stop_requested() && !checkpoints.error_occurred);
    if (checkpoints.error_occurred) {
      CORE_LOG_ERROR("Thread error occurred, exiting thread");
      /// handle error
      return;
    }

    set_current_state(SHUTTING_DOWN);
    if (checkpoints.error_occurred) {
      /// do something with errors, report them, attempt recovery?, etc...
    }

    set_current_state(STOPPED);
  }

  void thread::wait_for_initialization() {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    CORE_LOG_DEBUG("Thread [{}] waiting for initialization...", get_thread_name());

    do {
      opt<message> msg = thread_data->rx_channel->await_message(std::chrono::milliseconds(200));
      if (!msg.has_value()) {
        continue;
      }

      handle_init_msg(*msg);
    } while (!checkpoints.initialized && !checkpoints.error_occurred && !thread_data->stoken.stop_requested());

    acknowledgement ackmsg;
    ackmsg.acked_header = { CONTROL, THREAD_INITIALIZE };
    ackmsg.ack_nack = checkpoints.initialized ? 1 : 0;
    ackmsg.node_id = 0;  // not used here
    CORE_LOG_DEBUG("Thread [{}] initialization {}", get_thread_name(), ackmsg.ack_nack == 1 ? "successful" : "failed");

    message ack;
    ack.header = { ACKNOWLEDGEMENT, ACK };
    ack.data = ackmsg.build();

    checkpoints.error_occurred = false;
    thread_data->tx_channel->push(std::move(ack));
  }

  void thread::handle_init_msg(const message& msg) {
    /// use raw header bc custom message type
    if (msg.header.category == CONTROL && msg.header.id == THREAD_INITIALIZE) {
      checkpoints.initialized = true;
    } else {
      CORE_LOG_ERROR("Invalid message type for thread initialization : {}:{}\n", msg.get_category(), msg.get_id());
      thread_exit_code = -1;
      checkpoints.error_occurred = true;
    }
  }

  void thread::wait_for_start() {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");
    CORE_LOG_DEBUG("Thread [{}] waiting to start...", get_thread_name());

    do {
      opt<message> msg = thread_data->rx_channel->await_message(std::chrono::milliseconds(200));
      if (!msg) {
        continue;
      }
      handle_start_msg(*msg);
    } while (!checkpoints.running && !checkpoints.error_occurred && !thread_data->stoken.stop_requested());
  }

  void thread::handle_start_msg(const message& msg) {
    /// use raw header bc custom message type
    if (msg.header.category == CONTROL && msg.header.id == THREAD_START) {
      CORE_LOG_DEBUG("Thread [{}] started", get_thread_name());
      checkpoints.running = true;
      checkpoints.error_occurred = false;
    } else {
      CORE_LOG_ERROR("Invalid message type for thread start : {}:{}\n", msg.get_category(), msg.get_id());
      checkpoints.error_occurred = true;
      thread_exit_code = -1;
    }
  }

  void thread::wait_for_shutdown() {
    OTHER_ASSERT(thread_data != nullptr, "Thread data is null");

    acknowledgement ackmsg;
    ackmsg.acked_header = { CONTROL, THREAD_SHUTDOWN };
    ackmsg.ack_nack = checkpoints.finalized ? 1 : 0;
    ackmsg.node_id = 0;  // not used here

    message ack;
    ack.header = { ACKNOWLEDGEMENT, ACK };
    ack.data = ackmsg.build();

    thread_data->tx_channel->push(std::move(ack));
  }

  void thread::handle_shutdown_msg(const message& msg) {
    /// use raw header bc custom message type
    if (msg.header.id == THREAD_SHUTDOWN) {
      checkpoints.running = false;
    } else {
      CORE_LOG_DEBUG("Invalid message type for thread shutdown : {}:{}", msg.get_category(), msg.get_id());
      checkpoints.error_occurred = true;
      thread_exit_code = -1;
    }
  }

  void thread::handle_message(const message& msg) {
    switch (msg.get_category()) {
      case message_category::NOTIFICATION:
        handle_notification_message(msg);
        break;

      case message_category::ACKNOWLEDGEMENT:
        /// we know that the message is an acknowledgement message
        handle_acknowledgement_message(msg);
        break;

      case message_category::CONTROL:
        handle_control_message(msg);
        break;

      case message_category::COMMAND:
        handle_command_message(msg);
        break;

      case message_category::QUERY:
        handle_query_message(msg);
        break;

      case message_category::RESPONSE:
        handle_response_message(msg);
        break;

      case message_category::ERROR_ALERT:
        handle_error_alert_message(msg);
        break;

      case message_category::INFO:
        handle_info_message(msg);
        break;

      default:
        CORE_LOG_WARN("Terminal received unsupported message category: {}", msg.get_category());
        break;
    }
  }

  void thread::handle_notification_message(const message& msg) {
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported notification message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_acknowledgement_message(const message& msg) {
    OTHER_ASSERT(msg.get_category() == ACKNOWLEDGEMENT, "Invalid acknowledgement message category: {}", msg.header.category);
    OTHER_ASSERT(msg.get_id() == ACK, "Invalid acknowledgement message id: {}", msg.header.id);

    acknowledgement ack = acknowledgement::parse(msg.data);
    OTHER_ASSERT(ack.category == ACKNOWLEDGEMENT, "Invalid acknowledgement message category: {}", ack.acked_header.category);
    OTHER_ASSERT(ack.id == ACK, "Invalid acknowledgement message id: {}", ack.acked_header.id);

    handle_acknowledgement(ack);
  }

  void thread::handle_control_message(const message& msg) {
    switch (msg.get_id()) {
      case PING:
        handle_ping(session_status_request::parse(msg.data));
        break;

      case PONG:
        handle_pong(session_status_response::parse(msg.data));
        break;

      default:
        CORE_LOG_WARN("Thread received unsupported control message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_command_message(const message& msg) {
    switch (msg.get_id()) {
      case OTHER_COMMAND:
        handle_command(other_command_msg::parse(msg.data));
        break;

      case OTHER_COMMAND_BLOCK:
        handle_command_block(other_command_block_msg::parse(msg.data));
        break;

      default:
        CORE_LOG_WARN("Thread received unsupported command message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_query_message(const message& msg) {
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported query message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_response_message(const message& msg) {
    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported response message: {}", msg.get_id());
        break;
    }
  }

  void thread::handle_error_alert_message(const message& msg) {
    CORE_LOG_ERROR("Thread received error alert: {}", msg.get_id());

    switch (msg.get_id()) {
      default:
        break;
    }
  }

  void thread::handle_info_message(const message& msg) {
    CORE_LOG_INFO("Thread received info message: {}", msg.get_id());

    switch (msg.get_id()) {
      default:
        CORE_LOG_WARN("Thread received unsupported info message: {}", msg.get_id());
        break;
    }
  }

}  // namespace other