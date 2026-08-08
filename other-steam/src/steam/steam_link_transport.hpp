/**
 * \file steam/steam_link_transport.hpp
 **/
#ifndef OTHER_STEAM_STEAM_STEAM_LINK_TRANSPORT_HPP
#define OTHER_STEAM_STEAM_STEAM_LINK_TRANSPORT_HPP

#include "core/defines.hpp"

#include "peer_mesh/link_transport.hpp"

#include <steam/steam_api.h>

namespace other {

  /// SNS reliable sends cap at 512 KiB; join snapshots can exceed it. blobs ride as
  ///  [u8 flags: bit0 = final][u16 index le][payload <= 448 KiB] chunks reassembled
  ///  per connection — transport-internal "how bytes move", never a message schema
  constexpr size_t kSteamFragmentPayload = 448 * 1024;
  constexpr size_t kSteamFragmentHeader = 3;

  ostd::vector<ostd::vector<uint8_t>> fragment_blob(std::span<const uint8_t> blob, size_t max_payload = kSteamFragmentPayload);

  class fragment_accumulator {
   public:
    /// the completed blob, or nullopt while accumulating; malformed or out-of-order
    ///  chunks reset the accumulator (cannot happen on a reliable-ordered stream)
    opt<ostd::vector<uint8_t>> feed(std::span<const uint8_t> chunk);

   private:
    ostd::vector<uint8_t> pending;
    uint16_t next_index = 0;
  };

  /// the byte mover over ISteamNetworkingSockets P2P (SDR). main-thread home:
  ///  connection events arrive during SteamAPI_RunCallbacks, receives are polled by
  ///  pump(). construct only when the steam context is READY
  class steam_link_transport final : public link_transport {
   public:
    explicit steam_link_transport(int virtual_port);
    ~steam_link_transport() override;

    std::string_view name() const override { return "steam"; }
    bool is_stream() const override { return false; }
    link_caps conn_caps(natural_t conn_id) const override {
      return { .reliable = true, .ordered = true, .max_frame_size = 0 };
    }
    node_id attested_remote(natural_t conn_id) const override;

    void bind(callbacks cbs) override { consumer = std::move(cbs); }

    natural_t dial(const net_address& remote) override;
    natural_t listen(const net_address& bind_addr) override;
    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override;
    void close(natural_t conn_id) override;

    /// main thread, once per tick, after the context pumped callbacks
    void pump();

   private:
    callbacks consumer;
    int virtual_port = 0;
    HSteamNetPollGroup poll_group = 0;
    HSteamListenSocket listen_socket = 0;
    ostd::map<natural_t, fragment_accumulator> assembly;

    STEAM_CALLBACK(steam_link_transport, on_status_changed, SteamNetConnectionStatusChangedCallback_t);
  };

}  // namespace other

#endif  // OTHER_STEAM_STEAM_STEAM_LINK_TRANSPORT_HPP
