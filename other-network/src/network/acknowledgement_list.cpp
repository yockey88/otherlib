/**
 * \file network/acknowledgement_list.cpp
 **/
#include "network/acknowledgement_list.hpp"

#include "core/enum_formatter.hpp"
#include "core/profiler.hpp"

namespace other {

  natural_t acknowledgement_list::register_ack(asio::io_context& io, message_header header, microseconds timeout, message_handler handler) {
    PROFILE_SECTION("acknowledgement_list::register_ack");
    const natural_t ack_id = generate_ack_id();

    CORE_LOG_DEBUG("[PENDING-ACK : {}] (timeout: {} us)", header, timeout.count());
    pending_ack& entry = pending_acks.emplace_back(pending_ack{
      .id = ack_id,
      .header = header,
      .timeout_duration = timeout,
      .sent_time = std::chrono::steady_clock::now(),
      .handler = handler,
      .timer = make_scope<asio::steady_timer>(io),
    });

    entry.timer->expires_after(timeout);
    entry.timer->async_wait([this, id = ack_id](const asio::error_code& ec) {
      PROFILE_SECTION("acknowledgement_list::register_ack--timeout_handler");
      auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == id; });
      if (itr == pending_acks.end()) {
        /// already acked (handle_ack cancelled us and erased the entry) — nothing to do
        return;
      }

      if (ec) {
        if (ec != asio::error::operation_aborted) {
          CORE_LOG_ERROR("Error in acknowledgment timer for ACK ID {}: {}", id, ec.message());
        }
        pending_acks.erase(itr);
        return;
      }

      CORE_LOG_WARN("Acknowledgment timeout for message {} (ACK ID: {})", itr->header, id);
      if (itr->handler.on_timeout) {
        itr->handler.on_timeout(itr->header);
        /// the handler may have mutated the list (registering new acks) — re-find before reaping
        itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == id; });
        if (itr == pending_acks.end()) {
          return;
        }
      }
      /// timed-out entries always reap — a null on_timeout must not leak the entry
      pending_acks.erase(itr);
    });

    return ack_id;
  }

  void acknowledgement_list::handle_ack(natural_t ack_id, message_header original_header, std::span<const uint8_t> data) {
    PROFILE_SECTION("acknowledgement_list::handle_ack");
    auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id && ack.header == original_header; });
    if (itr == pending_acks.end()) {
      CORE_LOG_ERROR("Received acknowledgment for unknown message ID {} with ACK ID {}", original_header, ack_id);
      return;
    }

    CORE_LOG_DEBUG("[ACK RECEIVED: {}] (ACK ID: {})", original_header, ack_id);
    if (itr->handler.handle_msg) {
      itr->handler.handle_msg(original_header, data);
    }

    itr->timer->cancel();
    pending_acks.erase(itr);
  }

  void acknowledgement_list::handle_failure(natural_t ack_id, message_header original_header, std::span<const uint8_t> data) {
    PROFILE_SECTION("acknowledgement_list::handle_failure");
    auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id && ack.header == original_header; });
    if (itr == pending_acks.end()) {
      CORE_LOG_ERROR("Received failure acknowledgment for unknown message ID {} with ACK ID {}", original_header, ack_id);
      return;
    }

    CORE_LOG_WARN("[ACK FAILED: {}] (ACK ID: {})", original_header, ack_id);
    /// resolve the entry before invoking: the handler may register new acks and
    ///  invalidate deque iterators
    message_handler::handler_fn on_failure = std::move(itr->handler.on_failure);
    itr->timer->cancel();
    pending_acks.erase(itr);

    if (on_failure) {
      on_failure(original_header, data);
    }
  }

  void acknowledgement_list::clear() {
    PROFILE_SECTION("acknowledgement_list::clear");
    for (auto& ack : pending_acks) {
      ack.timer->cancel();
    }
    pending_acks.clear();
    pending_responses.clear();
  }

  size_t acknowledgement_list::pending_count() const {
    return pending_acks.size();
  }

  void acknowledgement_list::add_pending_ack_response(natural_t ack_id, message_header header) {
    pending_responses.push_back({ ack_id, header });
  }

  natural_t acknowledgement_list::get_pending_ack_response(message_header header) {
    auto itr = std::ranges::find_if(pending_responses, [&](const pending_response& resp) { return resp.header == header; });
    if (itr == pending_responses.end()) {
      return 0;
    }

    natural_t ack_id = itr->ack_id;
    pending_responses.erase(itr);
    return ack_id;
  }

}  // namespace other
