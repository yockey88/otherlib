/**
 * \file network/session/authored_session.hpp
 *
 * Mode 1 (authored, zero-code): the session-layer half the driver glue calls at
 * playback edges. header-only and driver-free so the sim fixtures exercise the
 * real path; the glue supplies config defaults and the steam establishment hook.
 **/
#ifndef OTHERLIB_NETWORK_SESSION_AUTHORED_SESSION_HPP
#define OTHERLIB_NETWORK_SESSION_AUTHORED_SESSION_HPP

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "object/network_component.hpp"
#include "object/network_settings_component.hpp"
#include "scene/scene.hpp"
#include "serialization/scene_serializer.hpp"

#include "network/session/network_session.hpp"
#include "network/session/replication.hpp"

namespace other {
  namespace authored_session {

    struct defaults {
      std::string transport = "tcp";
      uint16_t port = 49222;
      /// join dial for non-steam addressed sessions; tests hand the sim endpoint in
      std::function<net_address(std::string_view address, uint16_t port)> resolve_address;
      /// listen address for non-steam hosting; null = ip ANY on the resolved port
      std::function<net_address(uint16_t port)> resolve_listen;
      /// steam establishment (lobby + P2P listen); null = steam rows unavailable
      std::function<bool()> host_steam;
    };

    /// first-found wins, conventionally on a settings object (the scene root is
    ///  not captured, so root-attached settings would not survive save/load)
    inline const network_settings_component* find_settings(scene& s) {
      for (const natural_t object_id : s.get_all_object_ids()) {
        if (const network_settings_component* settings = s.get_component<network_settings_component>(object_id); settings != nullptr) {
          return settings;
        }
      }
      return nullptr;
    }

    /// applies the authored policy; true = a session started (or an invite-driven
    ///  join is armed). failure is a logged event, never a failed play
    inline bool start(network_session& session, const network_settings_component& settings, const defaults& engine) {
      if (settings.session_mode == "off") {
        return false;
      }
      if (settings.max_peers != 0) {
        session.set_max_peers(settings.max_peers);
      }
      const std::string transport = !settings.transport.empty() ? settings.transport : engine.transport;
      const uint16_t port = settings.port != 0 ? settings.port : engine.port;

      if (settings.session_mode == "host") {
        if (transport == "steam") {
          return engine.host_steam != nullptr && engine.host_steam();
        }
        return session.host(engine.resolve_listen != nullptr ? engine.resolve_listen(port)
                                                             : net_address::ip_endpoint({ 0, port }));
      }
      if (settings.session_mode == "join") {
        if (settings.address.empty()) {
          /// steam: the invite drives the join; anything else has nowhere to dial
          return transport == "steam";
        }
        std::string address = settings.address;
        uint16_t dial_port = port;
        if (const size_t colon = address.find(':'); colon != std::string::npos) {
          dial_port = static_cast<uint16_t>(std::atoi(address.c_str() + colon + 1));
          address.resize(colon);
        }
        if (engine.resolve_address == nullptr) {
          CORE_LOG_WARN("authored join has no address resolver");
          return false;
        }
        return session.join(engine.resolve_address(address, dial_port));
      }
      CORE_LOG_WARN("authored session-mode '{}' is not one of off|host|join", settings.session_mode);
      return false;
    }

    /// clones the named disabled template under the scene root, marks ownership,
    ///  and registers the clone for replication. 0 = template missing/clone failed
    inline natural_t spawn_template(scene& s, replication& repl, std::string_view template_name, uint16_t peer) {
      scene_object* template_root = s.find_object(template_name);
      if (template_root == nullptr) {
        CORE_LOG_WARN("spawn template '{}' not found in the active scene", template_name);
        return 0;
      }

      const serialization::scene_document doc =
        serialization::capture_object_subtree(s, template_root->id, serialization::default_codec_services());
      ostd::map<natural_t, natural_t> remap;
      serialization::instantiate_subtree(s, doc, nullptr, serialization::default_codec_services(), &remap);
      auto cloned = remap.find(template_root->id);
      if (cloned == remap.end()) {
        return 0;
      }

      scene_object* clone = s.find_object(cloned->second);
      clone->name = std::string(template_name) + "-p" + std::to_string(peer);
      clone->visible = true;  // templates are authored disabled
      if (network_component* net = s.get_component<network_component>(clone); net != nullptr) {
        net->owner_peer = peer;
      }
      repl.spawn_object(clone->id, peer);
      return clone->id;
    }

  }  // namespace authored_session
}  // namespace other

#endif  // OTHERLIB_NETWORK_SESSION_AUTHORED_SESSION_HPP
