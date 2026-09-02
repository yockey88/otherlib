/**
 * \file network/session/script_actor.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_SCRIPT_ACTOR_HPP
#define OTHERLIB_NETWORK_SESSION_SCRIPT_ACTOR_HPP

#include <functional>
#include <string>

#include "core/defines.hpp"

#include "peer_mesh/peer_actor.hpp"

namespace other {

  /// the managed-actor bridge: a primary whose behavior lives in C#. the glue injects
  ///  dispatch callbacks (named-dispatch + park-and-pull, the proven marshal patterns);
  ///  the shim itself is scripting-free, so tests drive it with plain lambdas
  class OTHER_CLASS script_actor final : public peer_actor {
   public:
    struct callbacks {
      std::function<void(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload)> frame;
      std::function<void(const link_record& link)> link_up;
      std::function<void(const link_record& link, link_close_reason reason)> link_down;
      std::function<void(microseconds now, double dt)> ticked;
    };

    script_actor(std::string display_name, std::string managed_type, callbacks hooks)
        : display(std::move(display_name)), type_name(std::move(managed_type)), hooks(std::move(hooks)) {}

    std::string_view name() const override { return display; }
    const std::string& managed_type() const { return type_name; }

    // void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override {
    //   if (hooks.frame) {
    //     hooks.frame(via, src, net_id, payload);
    //   }
    // }
    // void on_link_up(const link_record& link) override {
    //   if (hooks.link_up) {
    //     hooks.link_up(link);
    //   }
    // }
    // void on_link_down(const link_record& link, link_close_reason reason) override {
    //   if (hooks.link_down) {
    //     hooks.link_down(link, reason);
    //   }
    // }
    // void tick(microseconds now, double dt) override {
    //   if (hooks.ticked) {
    //     hooks.ticked(now, dt);
    //   }
    // }

   private:
    std::string display;
    std::string type_name;
    callbacks hooks;
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_SESSION_SCRIPT_ACTOR_HPP
