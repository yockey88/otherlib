/**
 * \file network/acknowledgement_list.cpp
 **/
#include "network/acknowledgement_list.hpp"

namespace other {

  void acknowledgement_list::poll() {
    clear_cancelled_acks();
    clear_finished_acks();
  }

  natural_t acknowledgement_list::register_ack(asio::io_context& io, message_header header, microseconds timeout, message_handler handler) {
    natural_t ack_id = generate_ack_id();
    OTHER_ASSERT(std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id && ack.header == header; }) == pending_acks.end(), "Acknowledgment for message ID {} already pending", header);

    CORE_LOG_DEBUG("[PENDING-ACK : {}] (timeout: {} us)", header, timeout.count());
    pending_ack ack{
      .id = ack_id,
      .header = header,
      .timeout_duration = timeout,
      .handler = handler,
      .timer = asio::steady_timer(io),
    };

    ack.sent_time = std::chrono::steady_clock::now();
    auto ack_itr = pending_acks.insert(pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);

    ack_itr->timer.expires_after(timeout);
    ack_itr->timer.async_wait([this, stime = ack.sent_time, id = ack_id](const asio::error_code& ec) {
      if (ec && ec == asio::error::operation_aborted) {
        queue_cancelled_ack_id(id);
        return;
      }

      if (!ec) {
        auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.sent_time == stime; });
        if (itr == pending_acks.end()) {
          CORE_LOG_ERROR("Failed to find ack for timeout callback!");
        }

        CORE_LOG_WARN("Acknowledgment timeout for message {}", itr->header);
        if (itr->handler.on_timeout) {
          itr->handler.on_timeout(itr->header);
        }
      } else {
        CORE_LOG_ERROR("Error in acknowledgment timer for message ID {}: {}", id, ec.message());
      }

      queue_cancelled_ack_id(id);
    });

    return ack_id;
  }

  void acknowledgement_list::handle_ack(natural_t ack_id, message_header original_header, std::span<const uint8_t> data) {
    auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id && ack.header == original_header; });
    if (itr == pending_acks.end()) {
      CORE_LOG_ERROR("Received acknowledgment for unknown message ID {} with ACK ID {}", original_header, ack_id);
      return;
    }

    CORE_LOG_DEBUG("[ACK RECEIVED: {}] (ACK ID: {})", original_header, ack_id);
    if (itr->handler.handle_msg) {
      itr->handler.handle_msg(original_header, data);
    }

    itr->timer.cancel();
    pending_acks.erase(itr);
  }

  void acknowledgement_list::cancel_ack(natural_t ack_id) {
    auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id; });
    if (itr != pending_acks.end()) {
      itr->timer.cancel();
      pending_acks.erase(itr);
    }
  }

  void acknowledgement_list::clear() {
    for (auto& ack : pending_acks) {
      ack.timer.cancel();
    }
    pending_acks.clear();
  }

  void acknowledgement_list::add_pending_ack_response(natural_t ack_id, message_header header) {
    pending_responses.push_back({ ack_id, header });
  }

  natural_t acknowledgement_list::get_pending_ack_response(message_header header) {
    auto itr = std::ranges::find_if(pending_responses, [&](const pending_response& resp) { return resp.header == header; });
    if (itr == pending_responses.end()) {
      return 0;
    }

    natural_t msg_id = itr->header.id;
    pending_responses.erase(itr);
    return msg_id;
  }

  void acknowledgement_list::clear_cancelled_acks() {
    for (natural_t ack_id : cancelled_ack_ids) {
      auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id; });
      OTHER_ASSERT(itr != pending_acks.end(), "Failed to find pending acknowledgment with ID {} to cancel", ack_id);
      pending_acks.erase(itr);
    }
    cancelled_ack_ids.clear();
  }

  void acknowledgement_list::clear_finished_acks() {
    for (natural_t ack_id : finish_ack_ids) {
      auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id; });
      OTHER_ASSERT(itr != pending_acks.end(), "Failed to find pending acknowledgment with ID {} to mark as finished", ack_id);
      pending_acks.erase(itr);
    }
    finish_ack_ids.clear();
  }

}  // namespace other