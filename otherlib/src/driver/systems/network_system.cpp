/**
 * \file driver/systems/network_system.cpp
 **/
#include "driver/systems/network_system.hpp"

#include "core/defines.hpp"

#include "driver/driver.hpp"

namespace other {

  void network_system::initialize(driver_kernel* kernel) {
    net_context = make_scope<network_context>();
    OTHER_ASSERT(net_context != nullptr, "Failed to create network context.");

    /// \todo move this somewhere more permanent?
    struct signal_catcher {
      signal_catcher(network_system* network_system_ptr)
          : network_system_ptr(network_system_ptr) {}
      void catch_signal(std::error_code ec, int signum) {
        if ((ec && ec == asio::error::operation_aborted) ||
            network_system_ptr == nullptr || network_system_ptr->net_context == nullptr) {
          return;
        }

        if (!ec) {
          network_system_ptr->catch_signal(signum);
        } else {
          CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());

          if (network_system_ptr->get_driver().current_driver_state() == driver_state::DRIVER_STATE_RUNNING) {
            network_system_ptr->net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, this));
          }
        }
      }

      network_system* network_system_ptr = nullptr;
    };

    static signal_catcher catcher{ this };
    net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, &catcher));

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);
      net_context->net_thread->launch();
      net_context->net_thread_message_bus.register_thread();
    }
  }

  void network_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    net_context->io_context.poll();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      auto msg_opt = net_context->net_thread_message_bus.receive_message();
      if (msg_opt.has_value()) {
        process_network_thread_messages(kernel, std::move(*msg_opt));
      }
    }
  }

  void network_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      net_context->net_thread->wait_for_shutdown_complete();
      net_context->net_thread = nullptr;
      net_context = nullptr;
    }
  }

  void network_system::begin_shutdown_sequence(driver_kernel* kernel) {
    net_context->signals.cancel();

    for (auto& ack : ack_list.pending_acks) {
      ack.timer.cancel();
    }

    CORE_LOG_DEBUG("Sending shutdown request to network thread...");
    message msg;
    msg.header = {
      .category = COMMAND,
      .id = SHUTDOWN_REQUEST,
    };

    send_message_and_wait_acknowledgment(
      kernel, std::move(msg), std::chrono::milliseconds(250),
      message_handler{
        [this, kernel](message_header h, std::span<const uint8_t> d) { on_ack_shutdown_request_network_thread(kernel, h, d); },
        [this, kernel](message_header h) { on_timeout_shutdown_request_network_thread(kernel, h); },
      }
    );
  }

  void network_system::send_to_network_thread(driver_kernel* kernel, message&& msg) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  natural_t network_system::open_tcp_connection(const binding_point& endpoint) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
  }

  void network_system::send_message_and_detach_acknowledgement(driver_kernel* kernel, message&& msg, message_handler handler) {
    acknowledgement_list::pending_ack ack{
      .header = msg.header,
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&ack](const acknowledgement_list::pending_ack& existing_ack) {
        return existing_ack.header == ack.header;
      });
      OTHER_ASSERT(itr == ack_list.pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header);
    }

    send_to_network_thread(kernel, std::move(msg));
    auto ack_itr = ack_list.pending_acks.insert(ack_list.pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != ack_list.pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);
  }

  natural_t network_system::send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
    acknowledgement_list::pending_ack ack{
      .header = msg.header,
      .timeout_duration = timeout,
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };
    CORE_LOG_DEBUG("PENDING-ACK {} (timeout: {} us)", ack.header, ack.timeout_duration.count());

    {
      auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&ack](const acknowledgement_list::pending_ack& existing_ack) {
        return existing_ack.header == ack.header;
      });
      OTHER_ASSERT(itr == ack_list.pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header);
    }

    ack.sent_time = std::chrono::steady_clock::now();
    ack.id = ack_list.next_pending_ack_id++;
    send_to_network_thread(kernel, std::move(msg));
    auto ack_itr = ack_list.pending_acks.insert(ack_list.pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != ack_list.pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);

    ack_itr->timer.expires_after(timeout);
    ack_itr->timer.async_wait([this, stime = ack.sent_time](const asio::error_code& ec) {
      if (ec && ec == asio::error::operation_aborted) {
        return;
      }

      if (!ec) {
        auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.sent_time == stime; });
        if (itr == ack_list.pending_acks.end()) {
          CORE_LOG_ERROR("Failed to find ack for timeout callback!");
        }

        CORE_LOG_WARN("Acknowledgment timeout for message {}", itr->header);
        if (itr->handler.on_timeout) {
          itr->handler.on_timeout(itr->header);
        }
      }

      // remove from pending acks
      auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.sent_time == stime; });
      if (itr != ack_list.pending_acks.end()) {
        CORE_LOG_DEBUG("Removing pending acknowledgment for message ID {}", itr->header.id);
        ack_list.pending_acks.erase(itr);
      }
    });

    return ack_itr->id;
  }

  void network_system::cancel_acknowledgment(natural_t ack_id) {
    auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.id == ack_id; });
    if (itr != ack_list.pending_acks.end()) {
      CORE_LOG_DEBUG("Cancelling pending acknowledgment for message ID {}", itr->header.id);
      itr->timer.cancel();
      ack_list.pending_acks.erase(itr);
    }
  }

  void network_system::catch_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down driver...", signum);
      get_driver().request_shutdown();
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signum);
    }
  }

  asio::io_context& network_system::io_context() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->io_context;
  }

  message_bus& network_system::net_message_bus() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->net_thread_message_bus;
  }

  bool network_system::network_active() const {
    return net_context->net_thread != nullptr && net_context->net_thread->is_running();
  }

  void network_system::process_network_thread_messages(driver_kernel* kernel, message&& msg) {
    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
          case NETWORK_THREAD_READY: handle_notification_network_thread_ready(kernel, std::move(msg)); break;
          case NETWORK_THREAD_SHUTDOWN_COMPLETE: handle_notification_network_thread_shutdown_complete(kernel, std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown notification message ID {}", msg.header.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (msg.header.id) {
          case ACK: handle_acknowledgement_ack(kernel, std::move(msg)); break;
          default: CORE_LOG_ERROR("Driver received unknown acknowledgment message ID {}", msg.header.id); break;
        }
        break;

      case CONTROL:
        switch (msg.header.id) {
          default:
            CORE_LOG_ERROR("Server received unknown CONTROL message ID {}", msg.header.id);
            break;
        }
        break;

      case COMMAND:
        switch (msg.header.id) {
          default:
            CORE_LOG_ERROR("Server received unknown COMMAND message ID {}", msg.header.id);
            break;
        }
        break;

      case REQUEST:
        switch (msg.header.id) {
          default:
            CORE_LOG_ERROR("Server received unknown REQUEST message ID {}", msg.header.id);
            break;
        }
        break;

      case RESPONSE: break;
      case ERROR_ALERT: get_driver().handle_error_alert(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.header.category);
        break;
    }
  }

  void network_system::on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");

    net_context->net_thread->shutdown();
  }

  void network_system::on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header) {
    /// force shutdown
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge shutdown request");
    CORE_LOG_ERROR("   Data may be corrupt from unclean shutdown");

    net_context->net_thread->force_shutdown();
  }

  void network_system::handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg) {
    get_driver().confirm_initialization();
  }

  void network_system::handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg) {
    net_context->net_thread->wait_for_shutdown_complete();
    CORE_LOG_DEBUG("Network thread has fully stopped.");

    net_context->io_context.stop();

    ack_list.pending_acks.clear();
    get_driver().confirm_network_thread_shutdown();
  }

  void network_system::handle_acknowledgement_ack(driver_kernel* kernel, message&& msg) {
    acknowledgement ackmsg = other_message_spec::parse<acknowledgement>(msg.data);
    message_header acked_header = ackmsg.acked_header;

    auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&](const acknowledgement_list::pending_ack& ack) {
      return acked_header == ack.header;
    });
    if (itr == ack_list.pending_acks.end()) {
      CORE_LOG_ERROR("Received acknowledgment for unknown message {}", acked_header);
      return;
    }

    itr->timer.cancel();
    if (ackmsg.ack_nack == 0x01) {
      CORE_LOG_DEBUG("  - ACK");
    } else {
      CORE_LOG_DEBUG("  - NACK");
    }

    if (itr->handler.handle_msg) {
      CORE_LOG_TRACE("Invoking acknowledgment callback for message {}", acked_header);
      itr->handler.handle_msg(acked_header, ackmsg.extra_data);
    }
  }
}  // namespace other