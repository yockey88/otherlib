/**
 * \file driver/systems/peer_mesh_system.cpp
 **/
#include "driver/systems/peer_mesh_system.hpp"

#include <random>
#include <sstream>

#include "core/subsystem.hpp"
#include "core/value.hpp"
#include "event/event_system.hpp"

#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "driver/systems/network_system.hpp"

namespace other {

  namespace {

    /// assembly-qualified: the managed side resolves this with Type.GetType
    constexpr std::string_view kNetworkClass = "Other.Networking.Network, OtherCs";

    bool parse_ip(std::string_view text, uint32_t& out_ip) {
      unsigned int a = 0, b = 0, c = 0, d = 0;
      if (std::sscanf(std::string(text).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
          a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
      }
      out_ip = (a << 24) | (b << 16) | (c << 8) | d;
      return true;
    }

  }  // namespace

  void peer_mesh_system::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel is null in peer_mesh_system::initialize.");
    PROFILE_SECTION("peer_mesh_system::initialize");
    networking_off = !get_driver().network_enabled();
  }

  void peer_mesh_system::late_initialize(driver_kernel* kernel) {
    PROFILE_SECTION("peer_mesh_system::late_initialize");
    event_system& events = *get_driver().get_event_system();
    events.register_event("network.session-started");
    events.register_event("network.session-ended");
    events.register_event("network.peer-joined");
    events.register_event("network.peer-left");
    events.register_event("network.command");
    events.add_listener("network.command", [this](const value& data) { handle_network_command(data); });

    auto register_actor_interface = [this](environment_registry& reg) {
      reg.register_interface<peer_mesh_actor>(
        std::function<natural_t(scope<peer_mesh_actor>)>([this](scope<peer_mesh_actor> actor) { return actors.provide(std::move(actor)); }),
        [this](natural_t id) {
          if (id != 0 && id == spawned_provider && driver_mesh != nullptr) {
            driver_mesh->destroy_actor(session_node);
            spawned_provider = 0;
          } else {
            actors.revoke(id);
          }
        },
        no_args(),
        interface_cardinality::MULTIPLE);
    };
    register_actor_interface(kernel->driver_registry());
    register_actor_interface(kernel->project_registry());
  }

  void peer_mesh_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("peer_mesh_system::tick");
    if (networking_off) {
      return;
    }
    engine_now += duration_cast<microseconds>(fseconds(dt));

    if (driver_mesh == nullptr) {
      build(kernel);
      return;
    }
    tcp_link->pump();
    driver_mesh->tick(engine_now);
  }

  void peer_mesh_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("peer_mesh_system::shutdown");
    if (driver_mesh == nullptr) {
      return;
    }
    /// mesh teardown still transmits BYEs — the adapter and network thread are
    ///  alive because this system shuts down ahead of them (reverse boot order)
    sibling<network_system>(*kernel).set_connection_taps(nullptr, nullptr);
    active_session = nullptr;
    driver_mesh = nullptr;
    tcp_link = nullptr;
  }

  void peer_mesh_system::build(driver_kernel* kernel) {
    network_system& net = sibling<network_system>(*kernel);
    if (!net.network_active()) {
      return;
    }
    transport_provider* tcp = net.find_provider("tcp");
    if (tcp == nullptr) {
      return;  // registered at confirm_initialization; try again next tick
    }
    PROFILE_SECTION("peer_mesh_system::build");

    peer_mesh_config cfg;
    cfg.handshake_timeout = milliseconds{ get_driver().get_config_value<size_t>("networking.handshake-timeout-ms", 3'000) };
    cfg.keepalive_idle = milliseconds{ get_driver().get_config_value<size_t>("networking.keepalive-idle-ms", 5'000) };
    cfg.link_timeout = milliseconds{ get_driver().get_config_value<size_t>("networking.link-timeout-ms", 15'000) };
    driver_mesh = make_scope<peer_mesh>("driver", cfg);

    tcp_link = make_scope<provider_link_transport>(*net.thread(), *tcp, link_caps{ .reliable = true, .ordered = true, .max_frame_size = 0 }, true);
    driver_mesh->register_transport(*tcp_link);
    net.set_connection_taps(
      [this](const notification_connection_opened& note) { tcp_link->on_connection_opened(note); },
      [this](const notification_connection_closed& note) { tcp_link->on_connection_closed(note); });

    /// D13 policy chain: config override, else random nonzero
    session_node = get_driver().get_config_value<size_t>("networking.node-id", 0);
    if (session_node == 0) {
      std::mt19937_64 gen{ std::random_device{}() };
      while (session_node == 0) {
        session_node = gen();
      }
    }

    const std::string actor_name = get_driver().get_config_value<std::string>("networking.session-host", std::string("client-server"));
    scope<peer_mesh_actor> actor;
    if (actor_name == network_session::kDefaultActorName) {
      network_session::session_config session_cfg;
      session_cfg.max_peers = static_cast<uint16_t>(get_driver().get_config_value<size_t>("networking.max-peers", 8));
      if (std::string project_name = get_driver().get_project_name(); !project_name.empty()) {
        session_cfg.display_name = std::move(project_name);
      }
      scope<network_session> session = make_scope<network_session>(session_cfg);
      session->set_observer([this](session_event ev, uint16_t arg) { on_session_event(ev, arg); });
      session->set_game_event_handler([this](uint16_t sender, std::string_view event_name, std::span<const uint8_t> payload) {
        dispatch_game_event(sender, event_name, payload);
      });
      active_session = session.get();
      actor = std::move(session);
    } else {
      session_actor_source::taken custom = actors.take(actor_name);
      if (custom.actor == nullptr) {
        CORE_LOG_ERROR("networking.session-host '{}' is unknown; networking is up with no session actor", actor_name);
        return;
      }
      spawned_provider = custom.provider_id;
      actor = std::move(custom.actor);
    }

    driver_mesh->spawn_actor(std::move(actor), session_node);
    CORE_LOG_INFO("Peer mesh up (node {:#018x}, session actor '{}')", session_node, actor_name);
  }

  bool peer_mesh_system::host_session(uint16_t port) {
    if (active_session == nullptr) {
      CORE_LOG_WARN("host refused: networking is disabled or a custom actor owns the session");
      return false;
    }
    if (port == 0) {
      port = static_cast<uint16_t>(get_driver().get_config_value<size_t>("networking.port", 49222));
    }
    return active_session->host(net_address::ip_endpoint({ 0, port }));
  }

  bool peer_mesh_system::join_session(const std::string_view address_text, uint16_t port) {
    if (active_session == nullptr) {
      CORE_LOG_WARN("join refused: networking is disabled or a custom actor owns the session");
      return false;
    }
    uint32_t ip = 0;
    if (!parse_ip(address_text, ip)) {
      CORE_LOG_WARN("join refused: '{}' is not a dotted-quad address", address_text);
      return false;
    }
    if (port == 0) {
      port = static_cast<uint16_t>(get_driver().get_config_value<size_t>("networking.port", 49222));
    }
    return active_session->join(net_address::ip_endpoint({ ip, port }));
  }

  size_t peer_mesh_system::copy_pending_event_payload(uint8_t* dst, size_t capacity) {
    const size_t count = std::min(capacity, pending_event_payload.size());
    std::copy_n(pending_event_payload.begin(), count, dst);
    return count;
  }

  void peer_mesh_system::handle_network_command(const value& data) {
    PROFILE_SECTION("peer_mesh_system::handle_network_command");
    if (data.type() != value_type::STRING) {
      return;
    }
    event_system& events = *get_driver().get_event_system();

    std::stringstream input{ data.as_string() };
    std::string verb, arg;
    input >> verb >> arg;

    if (verb == "host") {
      const uint16_t port = static_cast<uint16_t>(arg.empty() ? 0 : std::atoi(arg.c_str()));
      events.trigger_event("console.output", std::string(host_session(port) ? "hosting" : "host failed (see log)"));
    } else if (verb == "join") {
      std::string address = arg;
      uint16_t port = 0;
      if (const size_t colon = address.find(':'); colon != std::string::npos) {
        port = static_cast<uint16_t>(std::atoi(address.c_str() + colon + 1));
        address.resize(colon);
      }
      events.trigger_event("console.output", std::string(join_session(address, port) ? "joining..." : "join failed (see log)"));
    } else if (verb == "leave") {
      if (active_session != nullptr) {
        active_session->leave();
      }
      events.trigger_event("console.output", std::string("left session"));
    } else if (verb == "status") {
      events.trigger_event("console.output", status_text());
    } else {
      events.trigger_event("console.output", std::string("usage: net host [port] | join <ip[:port]> | leave | status"));
    }
  }

  std::string peer_mesh_system::status_text() const {
    if (driver_mesh == nullptr) {
      return "networking inactive";
    }
    std::stringstream out;
    if (active_session == nullptr || !active_session->in_session()) {
      out << "no session\n";
    } else {
      out << (active_session->is_host() ? "hosting" : "joined") << " as peer " << active_session->local_peer_id() << "\n";
      for (const session_member& member : active_session->peers()) {
        out << "  peer " << member.peer_id << "  '" << member.name << "'  node " << std::hex << member.node << std::dec << "\n";
      }
    }
    for (const link_record& link : driver_mesh->net().links()) {
      out << "  link " << link.link_id << "  state " << static_cast<uint32_t>(link.state)
          << "  rtt " << duration_cast<milliseconds>(link.rtt).count() << "ms"
          << "  tx " << link.stats.frames_tx << "/" << link.stats.bytes_tx << "B"
          << "  rx " << link.stats.frames_rx << "/" << link.stats.bytes_rx << "B\n";
    }
    return out.str();
  }

  void peer_mesh_system::on_session_event(session_event ev, uint16_t arg) {
    event_system& events = *get_driver().get_event_system();
    scripting_environment* scripts = subsystem<scripting_environment>::get();
    switch (ev) {
      case session_event::STARTED:
        events.trigger_event("network.session-started", static_cast<uint32_t>(arg));
        break;
      case session_event::ENDED:
        events.trigger_event("network.session-ended", static_cast<uint32_t>(arg));
        break;
      case session_event::PEER_JOINED:
        events.trigger_event("network.peer-joined", static_cast<uint32_t>(arg));
        if (scripts != nullptr) {
          scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchPeerJoined", arg);
        }
        break;
      case session_event::PEER_LEFT:
        events.trigger_event("network.peer-left", static_cast<uint32_t>(arg));
        if (scripts != nullptr) {
          scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchPeerLeft", arg);
        }
        break;
    }
  }

  void peer_mesh_system::dispatch_game_event(uint16_t sender_peer, std::string_view event_name, std::span<const uint8_t> payload) {
    scripting_environment* scripts = subsystem<scripting_environment>::get();
    if (scripts == nullptr) {
      return;
    }
    PROFILE_SECTION("peer_mesh_system::dispatch_game_event");
    /// payload parks here for the C# pull — the invoke marshal is primitives+string
    ///  only (house rule: no new marshal-table entries)
    pending_event_payload.assign(payload.begin(), payload.end());
    scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchEvent",
                                             sender_peer, native_string(event_name), static_cast<int32_t>(payload.size()));
    pending_event_payload.clear();
  }

}  // namespace other
