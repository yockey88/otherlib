/**
 * \file network/link_sink.cpp
 **/
#include "network/link_sink.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "message/message_serialization.hpp"

#include "network/mesh_messages.hpp"
#include "network/transport_provider.hpp"

namespace other {

  namespace {

    inline uint16_t control_id(mesh_message id) {
      return static_cast<uint16_t>(id);
    }

  }  // namespace

  link_sink::link_sink(transport_provider& provider, natural_t conn_id)
      : home(provider), connection(conn_id) {
    inline_delivery = provider.execution_home() == transport_home::MAIN_THREAD;
    stream = provider.is_stream();
    record.connection_id = conn_id;
    record.transport_hash = provider.hash();
    record.caps = provider.conn_caps(conn_id);
  }

  /// ---------------------------------------------------------------- provider surface

  void link_sink::rx_data(natural_t, std::span<const uint8_t> data) {
    deliver({ rx_event::kind::BYTES, ostd::vector<uint8_t>(data.begin(), data.end()) });
  }

  void link_sink::connection_opened(natural_t) {
    deliver({ rx_event::kind::OPENED, {} });
  }

  void link_sink::connection_closed(natural_t) {
    deliver({ rx_event::kind::CLOSED, {} });
  }

  void link_sink::deliver(rx_event&& ev) {
    if (inline_delivery && adopted()) {
      process_event(std::move(ev));
      return;
    }
    std::lock_guard lock(queue_mutex);
    queue.push_back(std::move(ev));
  }

  /// ---------------------------------------------------------------- mesh surface

  void link_sink::adopt(natural_t link_id, node_id local_seat, link_state initial, const settings& config,
                        link_security* security, mesh_counters* mesh_stats, events mesh_hooks, microseconds now) {
    OTHER_ASSERT(!adopted(), "link_sink for connection {} is already adopted (link {}).", connection, record.link_id);
    record.link_id = link_id;
    record.local = local_seat;
    record.state = initial;
    cfg = config;
    sec = security;
    counters = mesh_stats;
    hooks = std::move(mesh_hooks);

    reader = frame_reader{ cfg.max_frame_size };
    current_now = now;
    opened_at = now;
    last_rx = now;
    last_tx = now;

    if (record.state == link_state::HANDSHAKING) {
      /// accept side: the connection already exists — hello immediately
      send_hello();
    }
  }

  void link_sink::pump(microseconds now) {
    PROFILE_SECTION("link_sink::pump");
    current_now = now;
    pump_counter++;

    std::deque<rx_event> drained;
    {
      std::lock_guard lock(queue_mutex);
      drained.swap(queue);
    }
    for (rx_event& ev : drained) {
      if (record.state == link_state::DOWN) {
        return;
      }
      process_event(std::move(ev));
    }

    if (record.state != link_state::DOWN && record.state != link_state::DISCONNECTING) {
      run_timers(now);
    }
  }

  void link_sink::process_event(rx_event&& ev) {
    switch (ev.k) {
      case rx_event::kind::OPENED:
        if (record.state == link_state::CONNECTING) {
          record.state = link_state::HANDSHAKING;
          send_hello();
        }
        return;

      case rx_event::kind::BYTES:
        on_bytes(ev.bytes);
        return;

      case rx_event::kind::CLOSED:
        begin_close(link_close_reason::TRANSPORT_CLOSED, false);
        return;
    }
  }

  void link_sink::on_bytes(std::span<const uint8_t> bytes) {
    PROFILE_SECTION("link_sink::rx");
    last_rx = current_now;
    record.latest_rx_tick = pump_counter;
    record.stats.bytes_rx += bytes.size();

    if (stream) {
      reader.feed(bytes);
      while (record.state != link_state::DOWN && record.state != link_state::DISCONNECTING) {
        frame_parse_result result = reader.next();
        if (result.err == frame_parse_result::error::NEED_MORE) {
          return;
        }
        if (result.err != frame_parse_result::error::NONE) {
          if (counters != nullptr) {
            counters->malformed_frames++;
          }
          protocol_error("malformed or oversize stream frame");
          return;
        }
        process_frame(std::move(*result.frame));
      }
      return;
    }

    frame_parse_result result = parse_frame_exact(bytes, cfg.max_frame_size);
    if (result.err != frame_parse_result::error::NONE) {
      if (counters != nullptr) {
        counters->malformed_frames++;
      }
      protocol_error("delivery is not exactly one frame");
      return;
    }
    process_frame(std::move(*result.frame));
  }

  void link_sink::process_frame(parsed_frame&& frame) {
    record.stats.frames_rx++;

    if (is_mesh_control(frame.net_id)) {
      handle_control(frame.net_id, frame.payload);
      return;
    }

    if (record.state != link_state::UP) {
      protocol_error("application frame before link up");
      return;
    }

    /// decrypt first — everything above sees plaintext; control frames never get here
    if (sec != nullptr) {
      if (!sec->decrypt(record, std::span<uint8_t>(frame.payload))) {
        if (counters != nullptr) {
          counters->security_failures++;
        }
        begin_close(link_close_reason::SECURITY_ERROR, true);
        return;
      }
    }

    if (hooks.frame) {
      hooks.frame(record, frame.net_id, frame.payload);
    }
  }

  /// ---------------------------------------------------------------- control

  void link_sink::handle_control(uint16_t net_id, std::span<const uint8_t> payload) {
    try {
      switch (static_cast<mesh_message>(net_id)) {
        case mesh_message::LINK_HELLO:
          handle_hello(payload);
          return;

        case mesh_message::LINK_PING: {
          auto [ping, consumed] = deserialize_direct<mesh_link_ping>(payload);
          const mesh_link_pong pong{ .t_echo_us = ping.t_send_us };
          send_control(mesh_message::LINK_PONG, serialize_direct(pong));
          return;
        }

        case mesh_message::LINK_PONG: {
          auto [pong, consumed] = deserialize_direct<mesh_link_pong>(payload);
          if (!ping_outstanding || pong.t_echo_us != ping_token) {
            return;
          }
          ping_outstanding = false;
          const microseconds sample = current_now - ping_sent_at;
          record.rtt = record.rtt.count() == 0 ? sample : (record.rtt * 7 + sample) / 8;
          return;
        }

        case mesh_message::LINK_BYE: {
          auto [bye, consumed] = deserialize_direct<mesh_link_bye>(payload);
          CORE_LOG_TRACE("[LINK {}] remote bye (reason {})", record.link_id, bye.reason);
          begin_close(link_close_reason::REMOTE_BYE, false);
          return;
        }

        case mesh_message::LINK_AUTH: {
          if (sec == nullptr || record.state != link_state::AUTHENTICATING) {
            protocol_error("unexpected LINK_AUTH");
            return;
          }
          apply_auth_result(sec->on_auth_frame(*this, record, payload));
          return;
        }
      }
      protocol_error("unknown mesh control id");
    } catch (const std::exception& error) {
      CORE_LOG_WARN("[LINK {}] control parse failure: {}", record.link_id, error.what());
      protocol_error("control payload parse failure");
    }
  }

  void link_sink::handle_hello(std::span<const uint8_t> payload) {
    if (record.state != link_state::HANDSHAKING || remote_hello_valid) {
      protocol_error("unexpected LINK_HELLO");
      return;
    }

    auto [hello, consumed] = deserialize_direct<mesh_link_hello>(payload);
    if (hello.magic != kMeshMagic || hello.protocol != kMeshProtocolVersion) {
      protocol_error("hello magic/protocol mismatch");
      return;
    }
    if (hello.app_hash != cfg.app_hash) {
      protocol_error("hello app mismatch");
      return;
    }
    if (hello.node == 0) {
      protocol_error("hello carries invalid node id");
      return;
    }

    /// a transport that authenticates its remote pins the hello — a mismatched claim
    ///  is an identity failure, not a protocol slip
    if (const node_id attested = home.attested_remote(connection); attested != 0) {
      if (hello.node != attested) {
        if (counters != nullptr) {
          counters->security_failures++;
        }
        CORE_LOG_WARN("[LINK {}] hello claims node {:#x} but transport attests {:#x}", record.link_id, hello.node, attested);
        begin_close(link_close_reason::SECURITY_ERROR, true);
        return;
      }
      record.attested = true;
    }

    record.remote = hello.node;
    record.caps.reliable = record.caps.reliable && hello.reliable != 0;
    record.caps.ordered = record.caps.ordered && hello.ordered != 0;
    if (hello.max_frame_size != 0) {
      record.caps.max_frame_size = record.caps.max_frame_size == 0
        ? hello.max_frame_size
        : std::min(record.caps.max_frame_size, hello.max_frame_size);
    }
    remote_hello_valid = true;

    try_advance_past_handshake();
  }

  void link_sink::try_advance_past_handshake() {
    if (!hello_sent || !remote_hello_valid) {
      return;
    }
    if (sec == nullptr) {
      make_link_up();
      return;
    }
    record.state = link_state::AUTHENTICATING;
    apply_auth_result(sec->begin_auth(*this, record));
  }

  void link_sink::apply_auth_result(link_security::auth_result result) {
    switch (result) {
      case link_security::auth_result::PENDING:
        return;
      case link_security::auth_result::ESTABLISHED:
        make_link_up();
        return;
      case link_security::auth_result::FAILED:
        if (counters != nullptr) {
          counters->security_failures++;
        }
        begin_close(link_close_reason::SECURITY_ERROR, true);
        return;
    }
  }

  void link_sink::make_link_up() {
    record.state = link_state::UP;
    CORE_LOG_TRACE("[LINK {}] up ({} <-> {})", record.link_id, record.local, record.remote);
    if (hooks.link_up) {
      hooks.link_up(record);
    }
  }

  /// ---------------------------------------------------------------- tx

  void link_sink::send_hello() {
    if (hello_sent) {
      return;
    }
    const mesh_link_hello hello{
      .node = record.local,
      .app_hash = cfg.app_hash,
      .reliable = static_cast<uint8_t>(record.caps.reliable ? 1 : 0),
      .ordered = static_cast<uint8_t>(record.caps.ordered ? 1 : 0),
      .max_frame_size = cfg.max_frame_size,
    };
    hello_sent = true;
    send_control(mesh_message::LINK_HELLO, serialize_direct(hello));
    try_advance_past_handshake();
  }

  void link_sink::send_control(mesh_message id, std::span<const uint8_t> payload) {
    /// control frames skip filters and encryption by design
    const ostd::vector<uint8_t> frame = write_frame(control_id(id), payload);
    home.tx(connection, frame);
    last_tx = current_now;
    record.latest_tx_tick = pump_counter;
    record.stats.frames_tx++;
    record.stats.bytes_tx += frame.size();
  }

  void link_sink::send_auth(std::span<const uint8_t> blob) {
    send_control(mesh_message::LINK_AUTH, blob);
  }

  bool link_sink::tx(uint16_t net_id, std::span<const uint8_t> payload) {
    PROFILE_SECTION("link_sink::tx");
    if (record.state != link_state::UP) {
      if (counters != nullptr) {
        counters->refused_sends++;
      }
      return false;
    }

    ostd::vector<uint8_t> composed(payload.begin(), payload.end());
    if (sec != nullptr && !sec->encrypt(record, composed)) {
      if (counters != nullptr) {
        counters->security_failures++;
      }
      begin_close(link_close_reason::SECURITY_ERROR, true);
      return false;
    }

    const uint32_t frame_length = static_cast<uint32_t>(kFrameLengthFloor + composed.size());
    const uint32_t limit = record.caps.max_frame_size != 0 ? std::min(record.caps.max_frame_size, cfg.max_frame_size) : cfg.max_frame_size;
    if (frame_length > limit) {
      CORE_LOG_WARN("[LINK {}] refusing oversize send ({} > {})", record.link_id, frame_length, limit);
      if (counters != nullptr) {
        counters->refused_sends++;
      }
      return false;
    }

    const ostd::vector<uint8_t> frame = write_frame(net_id, composed);
    home.tx(connection, frame);
    last_tx = current_now;
    record.latest_tx_tick = pump_counter;
    record.stats.frames_tx++;
    record.stats.bytes_tx += frame.size();
    return true;
  }

  /// ---------------------------------------------------------------- lifecycle

  void link_sink::protocol_error(std::string_view what) {
    if (counters != nullptr) {
      counters->protocol_errors++;
    }
    CORE_LOG_WARN("[LINK {}] protocol error: {}", record.link_id, what);
    begin_close(link_close_reason::PROTOCOL_ERROR, true);
  }

  void link_sink::begin_close(link_close_reason reason, bool send_bye) {
    if (record.state == link_state::DOWN || record.state == link_state::DISCONNECTING) {
      return;
    }

    record.state = link_state::DISCONNECTING;
    if (send_bye) {
      /// best effort: the control path has no UP gate by construction
      const mesh_link_bye bye{ .reason = static_cast<uint16_t>(reason) };
      send_control(mesh_message::LINK_BYE, serialize_direct(bye));
    }

    home.close(connection);

    const link_record snapshot = record;
    record.state = link_state::DOWN;
    CORE_LOG_TRACE("[LINK {}] down (reason {})", record.link_id, static_cast<uint16_t>(reason));
    if (hooks.link_down) {
      hooks.link_down(snapshot, reason);
    }
  }

  void link_sink::run_timers(microseconds now) {
    switch (record.state) {
      case link_state::CONNECTING:
      case link_state::HANDSHAKING:
      case link_state::AUTHENTICATING:
        if (now - opened_at > cfg.handshake_timeout) {
          begin_close(link_close_reason::HANDSHAKE_TIMEOUT, true);
        }
        break;

      case link_state::UP:
        if (now - last_rx > cfg.link_timeout) {
          begin_close(link_close_reason::KEEPALIVE_TIMEOUT, true);
          break;
        }
        if (!ping_outstanding && now - last_tx > cfg.keepalive_idle) {
          ping_outstanding = true;
          ping_sent_at = now;
          ping_token = static_cast<uint64_t>(now.count());
          const mesh_link_ping ping{ .t_send_us = ping_token };
          send_control(mesh_message::LINK_PING, serialize_direct(ping));
        }
        break;

      case link_state::DISCONNECTING:
      case link_state::DOWN:
        break;
    }
  }

}  // namespace other
