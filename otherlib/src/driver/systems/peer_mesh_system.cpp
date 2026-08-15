/**
 * \file driver/systems/peer_mesh_system.cpp
 **/
#include "driver/systems/peer_mesh_system.hpp"

#include <random>
#include <sstream>

#include "core/enum_formatter.hpp"
#include "core/subsystem.hpp"
#include "core/value.hpp"
#include "event/event_system.hpp"

#include "network/session/authored_session.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "driver/systems/network_system.hpp"
#include "driver/systems/scene_system.hpp"

#include "steam/steam_context.hpp"

namespace other {

  namespace {

    /// assembly-qualified: the managed side resolves this with Type.GetType
    constexpr std::string_view kNetworkClass = "Other.Networking.Network, OtherCs";
    constexpr std::string_view kScriptActorClass = "Other.Networking.ScriptActor, OtherCs";

    bool parse_ip(std::string_view text, uint32_t& out_ip) {
      unsigned int a = 0, b = 0, c = 0, d = 0;
      if (std::sscanf(std::string(text).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
          a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
      }
      out_ip = (a << 24) | (b << 16) | (c << 8) | d;
      return true;
    }

    void append_mesh_status(std::stringstream& out, const peer_mesh& m) {
      const mesh_counters& counters = m.counters();
      out << "mesh '" << m.name() << "'  links " << m.link_count() << "  actors " << m.actor_count()
          << "  [refused " << counters.refused_sends << ", malformed " << counters.malformed_frames
          << ", no-actor " << counters.no_actor_drops << ", security " << counters.security_failures
          << ", protocol " << counters.protocol_errors << "]\n";
      for (const link_record& link : m.links()) {
        out << "  link " << link.link_id
            << "  " << magic_enum::enum_name(link.state) << (link.attested ? "+att" : "")
            << "  via " << m.transport_name(link.transport_hash)
            << "  node " << std::hex << link.remote << std::dec
            << "  rtt " << duration_cast<milliseconds>(link.rtt).count() << "ms"
            << "  tx " << link.stats.frames_tx << "/" << link.stats.bytes_tx << "B"
            << "  rx " << link.stats.frames_rx << "/" << link.stats.bytes_rx << "B\n";
      }
    }

  }  // namespace

  void peer_mesh_system::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel is null in peer_mesh_system::initialize.");
    PROFILE_SECTION("peer_mesh_system::initialize");
    // networking_off = !get_driver().network_enabled();
  }

  void peer_mesh_system::late_initialize(driver_kernel* kernel) {
    PROFILE_SECTION("peer_mesh_system::late_initialize");
    event_system& events = *get_driver().get_event_system();
    events.register_event("network.session-started");
    events.register_event("network.session-ended");
    events.register_event("network.peer-joined");
    events.register_event("network.peer-left");
    events.register_event("network.lobby-join-requested");
    events.register_event("network.command");
    events.add_listener("network.command", [this](const value& data) { handle_network_command(data); });

    auto register_actor_interface = [this](environment_registry& reg) {
      reg.register_interface<peer_actor>(
        [this](scope<peer_actor> actor) { return actors.provide(std::move(actor)); },
        [this](natural_t id) {
          if (id != 0 && id == spawned_provider && driver_mesh != nullptr) {
            driver_mesh->remove_actor(session_node);
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

    auto register_security_interface = [this](environment_registry& reg) {
      reg.register_interface<link_security>(
        [this](scope<link_security> sec) {
          provided_security.emplace_back(next_security_id, std::move(sec));
          return next_security_id++;
        },
        [this](natural_t id) {
          if (id != 0 && id == installed_security_provider) {
            /// the layer is installed on live links; it cannot be torn out mid-flight
            CORE_LOG_ERROR("the active networking.security provider unloaded; restart networking before further use");
            installed_security_provider = 0;
            return;
          }
          std::erase_if(provided_security, [id](const auto& entry) { return entry.first == id; });
        },
        no_args(),
        interface_cardinality::MULTIPLE);
    };
    register_security_interface(kernel->driver_registry());
    register_security_interface(kernel->project_registry());
  }

  void peer_mesh_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("peer_mesh_system::tick");
    if (networking_off) {
      return;
    }
    engine_now += duration_cast<microseconds>(fseconds(dt));

    /// the application's other logical networks tick regardless of the driver mesh
    for (auto& [name, extra] : extra_meshes) {
      extra->tick(engine_now);
    }

    if (driver_mesh == nullptr) {
      build(kernel);
      return;
    }

    if (steam_link != nullptr) {
      steam_link->pump();
    }

    driver_mesh->tick(engine_now);
    if (scene_replication != nullptr) {
      scene_replication->tick(engine_now);
    }
    watch_authored_playback(kernel);
  }

  void peer_mesh_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("peer_mesh_system::shutdown");
    extra_meshes.clear();
    active_script = nullptr;
    if (driver_mesh == nullptr) {
      return;
    }
    /// mesh teardown still transmits BYEs — the providers and network thread are
    ///  alive because this system shuts down ahead of them (reverse boot order)
    scene_ops = nullptr;
    scene_replication = nullptr;
    active_session = nullptr;
    steam_ctx = nullptr;
    driver_mesh = nullptr;
    steam_link = nullptr;
  }

  peer_mesh& peer_mesh_system::create_mesh(std::string_view mesh_name, const peer_mesh_config& cfg) {
    OTHER_ASSERT(!networking_off, "create_mesh: networking is force-disabled.");
    OTHER_ASSERT(mesh_name != "driver", "The name 'driver' is reserved for the session mesh.");
    OTHER_ASSERT(mesh(mesh_name) == nullptr, "A mesh named '{}' already exists.", mesh_name);
    extra_meshes.emplace_back(std::string(mesh_name), make_scope<peer_mesh>(mesh_name, cfg));
    return *extra_meshes.back().second;
  }

  peer_mesh* peer_mesh_system::mesh(std::string_view mesh_name) {
    if (mesh_name == "driver") {
      return driver_mesh.get();
    }
    for (auto& [name, extra] : extra_meshes) {
      if (name == mesh_name) {
        return extra.get();
      }
    }
    return nullptr;
  }

  void peer_mesh_system::destroy_mesh(std::string_view mesh_name) {
    std::erase_if(extra_meshes, [&](const auto& entry) { return entry.first == mesh_name; });
  }

  bool peer_mesh_system::script_send(node_id dst, uint16_t net_id) {
    if (active_script == nullptr || !active_script->spawned()) {
      return false;
    }
    return active_script->send(dst, net_id, staged_actor_payload);
  }

  natural_t peer_mesh_system::script_open_link(std::string_view address, uint16_t port, std::string_view transport) {
    if (active_script == nullptr || !active_script->spawned()) {
      return 0;
    }
    uint32_t ip = 0;
    if (!parse_ip(address, ip)) {
      return 0;
    }
    return active_script->open_link(net_address::ip_endpoint({ ip, port }), transport);
  }

  natural_t peer_mesh_system::script_open_listener(uint16_t port, std::string_view transport) {
    if (active_script == nullptr || !active_script->spawned()) {
      return 0;
    }
    return active_script->open_listener(net_address::ip_endpoint({ 0, port }), transport);
  }

  void peer_mesh_system::script_close_link(natural_t link_id, uint16_t reason) {
    if (active_script != nullptr && active_script->spawned()) {
      active_script->close_link(link_id, static_cast<link_close_reason>(reason));
    }
  }

  bool peer_mesh_system::host_session(uint16_t port) {
    if (active_session == nullptr) {
      CORE_LOG_WARN("host refused: networking is disabled or a custom actor owns the session");
      return false;
    }

    /// the physics.backend idiom: config picks the listen transport at host time
    if (get_driver().get_config_value<std::string>("networking.transport", std::string("tcp")) == "steam") {
      return host_steam_session();
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

  bool peer_mesh_system::host_steam_session() {
    if (active_session == nullptr || steam_ctx == nullptr) {
      CORE_LOG_WARN("steam host refused: steam is not ready");
      return false;
    }
    if (!active_session->host(net_address{ .addressing = net_address::kind::STEAM_PEER })) {
      return false;
    }

    const std::string type_name = get_driver().get_config_value<std::string>("networking.steam.lobby-type", std::string("friends"));
    uint8_t lobby_type = 1;  // friends
    if (type_name == "private") {
      lobby_type = 0;
    } else if (type_name == "public") {
      lobby_type = 2;
    } else if (type_name == "invisible") {
      lobby_type = 3;
    }
    const int max_members = static_cast<int>(get_driver().get_config_value<size_t>("networking.max-peers", 8));
    /// lobby failure downgrades to invite-less P2P hosting, already logged
    steam_ctx->lobby().create(lobby_type, max_members);
    return true;
  }

  bool peer_mesh_system::join_lobby(uint64_t lobby_id) {
    if (active_session == nullptr || steam_ctx == nullptr) {
      CORE_LOG_WARN("lobby join refused: steam is not ready");
      return false;
    }
    /// async: LobbyEnter resolves the owner, the ready_to_connect hook dials it
    return steam_ctx->lobby().join(lobby_id);
  }

  void peer_mesh_system::open_invite_dialog() {
    if (steam_ctx != nullptr) {
      steam_ctx->lobby().open_invite_dialog();
    }
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
      bool success = host_session(port);
      events.trigger_event("console.output", std::string(success ? "hosting..." : "host failed (see log)"));
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
    } else if (verb == "invite") {
      // open_invite_dialog();
    } else if (verb == "status") {
      events.trigger_event("console.output", status_text());
    } else if (verb == "meshes") {
      std::stringstream out;
      if (driver_mesh != nullptr) {
        append_mesh_status(out, *driver_mesh);
      } else {
        out << "driver mesh not built\n";
      }

      for (const auto& [name, extra] : extra_meshes) {
        append_mesh_status(out, *extra);
      }
      events.trigger_event("console.output", out.str());
    } else if (verb == "journal") {
      events.trigger_event("console.output", journal_text());
    } else {
      events.trigger_event("console.output", std::string("usage: net host [port] | join <ip[:port]> | invite | leave | status | meshes | journal"));
    }
  }

  void peer_mesh_system::build(driver_kernel* kernel) {
    network_system& net = sibling<network_system>(*kernel);
    if (!net.network_active()) {
      return;
    }

    transport_provider* tcp = net.find_provider("tcp");
    transport_provider* udp = net.find_provider("udp");
    transport_provider* memory = net.find_provider("memory");
    if (tcp == nullptr || udp == nullptr || memory == nullptr) {
      return;  // registered at confirm_initialization; try again next tick
    }
    PROFILE_SECTION("peer_mesh_system::build");

    peer_mesh_config cfg;
    cfg.handshake_timeout = milliseconds{ get_driver().get_config_value<size_t>("networking.handshake-timeout-ms", 3'000) };
    cfg.keepalive_idle = milliseconds{ get_driver().get_config_value<size_t>("networking.keepalive-idle-ms", 5'000) };
    cfg.link_timeout = milliseconds{ get_driver().get_config_value<size_t>("networking.link-timeout-ms", 15'000) };
    cfg.max_links = get_driver().get_config_value<size_t>("networking.max-links", 32);
    cfg.max_frame_size = static_cast<uint32_t>(get_driver().get_config_value<size_t>("networking.max-frame-bytes", kDefaultMaxFrameSize));
    driver_mesh = make_scope<peer_mesh>("driver", cfg);

    /// D20 made resolvable: a registry-provided layer, picked by name; "none" = no object
    const std::string security_name = get_driver().get_config_value<std::string>("networking.security", std::string("none"));
    if (security_name != "none") {
      if (scope<link_security> sec = take_security(security_name); sec != nullptr) {
        driver_mesh->set_security(std::move(sec));
      } else {
        CORE_LOG_WARN("networking.security '{}' is not provided by any plugin; continuing without a security layer", security_name);
      }
    }

    driver_mesh->attach_provider(*tcp);
    driver_mesh->attach_provider(*udp);
    driver_mesh->attach_provider(*memory);

    if (steam_context* steam = net.steam(); steam != nullptr && steam->state() == steam_state::READY) {
      steam_ctx = steam;
      steam_link = make_scope<steam_transport_provider>(static_cast<int>(get_driver().get_config_value<size_t>("networking.steam.virtual-port", 0)));
      driver_mesh->attach_provider(*steam_link);
      /// steam replaces dial and discovery, never the protocol: the lobby's only
      ///  outputs are a host id to dial and an invite to consider
      steam->lobby().set_hooks({
        .ready_to_connect = [this](uint64_t host_id) {
          if (active_session != nullptr) {
            active_session->join(net_address{ .addressing = net_address::kind::STEAM_PEER, .id = host_id });
          } },
        .join_requested = [this](uint64_t lobby_id) { handle_lobby_join_request(lobby_id); },
      });
    }

    /// D13 policy chain: attested identity (steam) beats config override beats random
    session_node = get_driver().get_config_value<size_t>("networking.node-id", 0);
    if (steam_ctx != nullptr) {
      session_node = steam_ctx->local_steam_id();
    }
    if (session_node == 0) {
      std::mt19937_64 gen{ std::random_device{}() };
      while (session_node == 0) {
        session_node = gen();
      }
    }

    scope<peer_actor> actor;
    const std::string actor_name = get_driver().get_config_value<std::string>("networking.session-host", std::string{ network_session::kDefaultActorName });
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

      replication_config repl_cfg;
      repl_cfg.snapshot_hz = static_cast<double>(get_driver().get_config_value<size_t>("networking.snapshot-hz", 20));
      repl_cfg.interp_delay = milliseconds{ get_driver().get_config_value<size_t>("networking.interp-delay-ms", 100) };
      scene_replication = make_scope<replication>(*active_session, [this] { return sibling<scene_system>(get_driver().get_kernel()).get_active_scene(); }, repl_cfg);

      scene_replication->set_script_field_hooks(
        [this](natural_t object_id) -> ostd::vector<uint8_t> {
          scripting_environment* scripts = subsystem<scripting_environment>::get();
          if (scripts == nullptr) {
            return {};
          }
          staged_script_fields.clear();
          scripts->call_static_dotnet_method<int32_t>(kNetworkClass, "CollectReplicatedFields", static_cast<uint64_t>(object_id));
          return std::move(staged_script_fields);
        },
        [this](natural_t object_id, std::span<const uint8_t> payload) {
          scripting_environment* scripts = subsystem<scripting_environment>::get();
          if (scripts == nullptr) {
            return;
          }
          pending_event_payload.assign(payload.begin(), payload.end());
          scripts->call_static_dotnet_method<void>(kNetworkClass, "ApplyReplicatedFields",
                                                   static_cast<uint64_t>(object_id), static_cast<int32_t>(payload.size()));
          pending_event_payload.clear();
        });

      scene_ops = make_scope<op_channel>(*active_session, [this] { return scene_replication->host_tick(); }, get_driver().get_config_value<size_t>("networking.op-journal-cap", 4096));
      scene_ops->set_validator([this](uint16_t peer, const scene_op& op) { return validate_op_via_scripts(peer, op); });
      scene_ops->set_applied_handler([this](const scene_op& op) {
        dispatch_op_to_scripts("DispatchOpApplied", op.actor, std::string(op.name.begin(), op.name.end()), op.subject, op.payload);
      });
      scene_ops->set_rejected_handler([this](std::string_view op_name, uint16_t reason) {
        if (scripting_environment* scripts = subsystem<scripting_environment>::get(); scripts != nullptr) {
          scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchOpRejected", native_string(op_name), reason);
        }
      });
    } else if (actor_name.rfind("script:", 0) == 0) {
      actor = make_script_actor(actor_name);
    } else {
      session_actor_source::taken custom = actors.take(actor_name);
      if (custom.actor == nullptr) {
        CORE_LOG_ERROR("networking.session-host '{}' is unknown; networking is up with no session actor", actor_name);
        return;
      }
      spawned_provider = custom.provider_id;
      actor = std::move(custom.actor);
    }

    driver_mesh->set_primary(std::move(actor), session_node);
    CORE_LOG_INFO("Peer mesh up (node {:#018x}, session actor '{}')", session_node, actor_name);
  }

  scope<peer_actor> peer_mesh_system::make_script_actor(const std::string& spec) {
    std::string type_name = spec.substr(7);  // past "script:"
    if (scripting_environment* scripts = subsystem<scripting_environment>::get(); scripts != nullptr) {
      scripts->call_static_dotnet_method<void>(kScriptActorClass, "Bind", native_string(type_name));
    } else {
      CORE_LOG_WARN("script actor '{}' requested with no scripting environment; callbacks will not dispatch", type_name);
    }

    script_actor::callbacks hooks{
      .frame = [this](const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) {
        if (scripting_environment* env = subsystem<scripting_environment>::get(); env != nullptr) {
          pending_event_payload.assign(payload.begin(), payload.end());
          env->call_static_dotnet_method<void>(kScriptActorClass, "DispatchActorFrame",
                                               static_cast<uint64_t>(src), static_cast<uint32_t>(net_id),
                                               static_cast<int32_t>(payload.size()));
          pending_event_payload.clear();
        } },
      .link_up = [](const link_record& link) {
        if (scripting_environment* env = subsystem<scripting_environment>::get(); env != nullptr) {
          env->call_static_dotnet_method<void>(kScriptActorClass, "DispatchActorLinkUp",
                                               static_cast<uint64_t>(link.link_id), static_cast<uint64_t>(link.remote));
        } },
      .link_down = [](const link_record& link, link_close_reason reason) {
        if (scripting_environment* env = subsystem<scripting_environment>::get(); env != nullptr) {
          env->call_static_dotnet_method<void>(kScriptActorClass, "DispatchActorLinkDown",
                                               static_cast<uint64_t>(link.link_id), static_cast<uint32_t>(reason));
        } },
      .ticked = [](microseconds, double dt) {
        if (scripting_environment* env = subsystem<scripting_environment>::get(); env != nullptr) {
          env->call_static_dotnet_method<void>(kScriptActorClass, "DispatchActorTick", dt);
        } },
    };
    scope<script_actor> script = make_scope<script_actor>(spec, std::move(type_name), std::move(hooks));
    active_script = script.get();
    return script;
  }

  scope<link_security> peer_mesh_system::take_security(std::string_view security_name) {
    for (auto itr = provided_security.begin(); itr != provided_security.end(); ++itr) {
      if (itr->second != nullptr && itr->second->name() == security_name) {
        scope<link_security> taken = std::move(itr->second);
        installed_security_provider = itr->first;
        provided_security.erase(itr);
        return taken;
      }
    }
    return nullptr;
  }

  void peer_mesh_system::handle_lobby_join_request(uint64_t lobby_id) {
    event_system& events = *get_driver().get_event_system();
    events.trigger_event("network.lobby-join-requested", static_cast<uint64_t>(lobby_id));
    if (scripting_environment* scripts = subsystem<scripting_environment>::get(); scripts != nullptr) {
      scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchLobbyJoinRequested", lobby_id);
    }
    if (get_driver().get_config_value<bool>("networking.steam.auto-join-invites", true)) {
      join_lobby(lobby_id);
    }
  }

  void peer_mesh_system::watch_authored_playback(driver_kernel* kernel) {
    scene* active = sibling<scene_system>(*kernel).get_active_scene();
    const bool playing = active != nullptr && active->is_playing();
    if (playing == scene_was_playing) {
      return;
    }
    scene_was_playing = playing;

    if (playing) {
      if (active_session != nullptr && !active_session->in_session()) {
        apply_authored_settings(*active);
      }
      return;
    }
    /// authored sessions follow their scene out of play; console/API sessions don't
    if (authored_active && active_session != nullptr) {
      active_session->leave();
    }
    authored_active = false;
    authored_spawn_template.clear();
  }

  void peer_mesh_system::apply_authored_settings(scene& s) {
    PROFILE_SECTION("peer_mesh_system::apply_authored_settings");
    const network_settings_component* settings = authored_session::find_settings(s);
    if (settings == nullptr || settings->session_mode == "off") {
      return;
    }
    authored_spawn_template = settings->spawn_template;

    const authored_session::defaults engine{
      .transport = get_driver().get_config_value<std::string>("networking.transport", std::string("tcp")),
      .port = static_cast<uint16_t>(get_driver().get_config_value<size_t>("networking.port", 49222)),
      .resolve_address = [](std::string_view address, uint16_t port) -> net_address {
        uint32_t ip = 0;
        parse_ip(address, ip);
        return net_address::ip_endpoint({ ip, port });
      },
      .host_steam = [this] { return host_steam_session(); },
    };
    authored_active = authored_session::start(*active_session, *settings, engine);
    if (!authored_active) {
      CORE_LOG_WARN("authored session did not start (mode '{}')", settings->session_mode);
    }
  }

  void peer_mesh_system::spawn_template_for_peer(uint16_t peer) {
    scene* s = sibling<scene_system>(get_driver().get_kernel()).get_active_scene();
    if (s == nullptr || scene_replication == nullptr) {
      return;
    }
    PROFILE_SECTION("peer_mesh_system::spawn_template_for_peer");
    const natural_t clone_id = authored_session::spawn_template(*s, *scene_replication, authored_spawn_template, peer);
    if (clone_id != 0) {
      CORE_LOG_INFO("spawned '{}' for peer {}", s->find_object(clone_id)->name, peer);
    }
  }

  void peer_mesh_system::on_session_event(session_event ev, uint16_t arg) {
    if (scene_replication != nullptr) {
      scene_replication->on_session_event(ev, arg);
    }
    if (scene_ops != nullptr) {
      scene_ops->on_session_event(ev, arg);
    }
    event_system& events = *get_driver().get_event_system();
    scripting_environment* scripts = subsystem<scripting_environment>::get();
    switch (ev) {
      case session_event::STARTED:
        events.trigger_event("network.session-started", static_cast<uint32_t>(arg));
        break;
      case session_event::ENDED:
        events.trigger_event("network.session-ended", static_cast<uint32_t>(arg));
        if (steam_ctx != nullptr) {
          steam_ctx->lobby().leave();
        }
        break;
      case session_event::PEER_JOINED:
        events.trigger_event("network.peer-joined", static_cast<uint32_t>(arg));
        if (scripts != nullptr) {
          scripts->call_static_dotnet_method<void>(kNetworkClass, "DispatchPeerJoined", arg);
        }
        /// Mode 1: authored spawn template clones for the new member (after the
        ///  replication forward above — the join snapshot precedes the clone)
        if (active_session != nullptr && active_session->is_host() && !authored_spawn_template.empty()) {
          spawn_template_for_peer(arg);
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

  op_result peer_mesh_system::validate_op_via_scripts(uint16_t peer, const scene_op& op) {
    scripting_environment* scripts = subsystem<scripting_environment>::get();
    if (scripts == nullptr) {
      return {};  // no gameplay loaded: accept — the envelope is audit either way
    }
    PROFILE_SECTION("peer_mesh_system::validate_op_via_scripts");
    pending_event_payload.assign(op.payload.begin(), op.payload.end());
    const nbool32 accepted = scripts->call_static_dotnet_method<nbool32>(
      kNetworkClass, "DispatchOpRequest", peer, native_string(std::string_view(reinterpret_cast<const char*>(op.name.data()), op.name.size())),
      static_cast<uint64_t>(op.subject), static_cast<int32_t>(op.payload.size()));
    pending_event_payload.clear();
    return { .accepted = accepted != 0, .reason = accepted != 0 ? uint16_t{ 0 } : uint16_t{ 1 } };
  }

  void peer_mesh_system::dispatch_op_to_scripts(std::string_view method, uint16_t actor, const std::string& op_name,
                                                natural_t subject, std::span<const uint8_t> payload) {
    scripting_environment* scripts = subsystem<scripting_environment>::get();
    if (scripts == nullptr) {
      return;
    }
    PROFILE_SECTION("peer_mesh_system::dispatch_op_to_scripts");
    pending_event_payload.assign(payload.begin(), payload.end());
    scripts->call_static_dotnet_method<void>(kNetworkClass, method, actor, native_string(op_name),
                                             static_cast<uint64_t>(subject), static_cast<int32_t>(payload.size()));
    pending_event_payload.clear();
  }

  std::string peer_mesh_system::status_text() {
    if (driver_mesh == nullptr) {
      return "networking inactive";
    }

    std::stringstream out;
    if (steam_ctx != nullptr) {
      out << "steam: ready (" << steam_ctx->persona_name() << ")\n";
      if (steam_ctx->lobby().state() != lobby_state::IDLE) {
        out << "lobby " << steam_ctx->lobby().lobby_id()
            << "  " << magic_enum::enum_name(steam_ctx->lobby().state())
            << "  " << steam_ctx->lobby().member_count() << " members\n";
      }
    } else if (sibling<network_system>(get_driver().get_kernel()).steam() != nullptr) {
      out << "steam: unavailable\n";
    }

    if (active_session == nullptr || !active_session->in_session()) {
      out << "no session\n";
    } else {
      out << (active_session->is_host() ? "hosting" : "joined") << " as peer " << active_session->local_peer_id() << "\n";
      for (const session_member& member : active_session->peers()) {
        out << "  peer " << member.peer_id << "  '" << member.name << "'  node " << std::hex << member.node << std::dec << "\n";
      }
    }
    if (active_script != nullptr) {
      out << "script actor '" << active_script->managed_type() << "'\n";
    }

    append_mesh_status(out, *driver_mesh);
    for (const auto& [name, extra] : extra_meshes) {
      append_mesh_status(out, *extra);
    }

    if (scene_replication != nullptr) {
      out << "replication: host tick " << scene_replication->host_tick() << "\n";
    }
    if (scene_ops != nullptr) {
      out << "ops: journal " << scene_ops->journal().size() << " entries\n";
    }
    return out.str();
  }

  std::string peer_mesh_system::journal_text() {
    if (scene_ops == nullptr) {
      return "networking inactive";
    }
    std::stringstream out;
    out << "op journal (" << scene_ops->journal().size() << " entries)\n";
    for (const scene_op& op : scene_ops->journal()) {
      out << "  #" << op.op_id << "  tick " << op.tick << "  peer " << op.actor
          << "  '" << std::string(op.name.begin(), op.name.end()) << "'"
          << "  subject " << op.subject << "  " << op.payload.size() << "B\n";
    }
    return out.str();
  }

}  // namespace other
