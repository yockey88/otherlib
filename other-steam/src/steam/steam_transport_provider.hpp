/**
 * \file steam/steam_transport_provider.hpp
 **/
#ifndef OTHER_STEAM_STEAM_STEAM_TRANSPORT_PROVIDER_HPP
#define OTHER_STEAM_STEAM_STEAM_TRANSPORT_PROVIDER_HPP

#include "core/defines.hpp"

#include "network/transport_provider.hpp"

#include <steam/steam_api.h>

namespace other {

  /// SNS reliable sends cap at 512 KiB; join snapshots can exceed it, so blobs ride as [u8 flags]
  ///  [u16 index][payload <= 448 KiB] chunks reassembled per connection — transport-internal, not a schema
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

  /// the byte mover over ISteamNetworkingSockets P2P (SDR). main-thread home: connection events
  ///  arrive during SteamAPI_RunCallbacks, receives are polled by pump(). construct only when READY
  class steam_transport_provider final : public transport_provider {
   public:
    explicit steam_transport_provider(int virtual_port);
    ~steam_transport_provider() override;

    std::string name() const override { return "steam"; }
    transport_home execution_home() const override { return transport_home::MAIN_THREAD; }
    bool is_stream() const override { return false; }
    link_caps conn_caps(natural_t) const override {
      return { .reliable = true, .ordered = true, .max_frame_size = 0 };
    }
    node_id attested_remote(natural_t conn_id) const override;

    natural_t dial(const net_address& remote) override;
    natural_t listen(const net_address& bind_addr, accept_delegate on_accept) override;
    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override;
    void close(natural_t conn_id) override;

    /// main thread, once per tick, after the context pumped callbacks
    void pump();

   private:
    int virtual_port = 0;
    HSteamNetPollGroup poll_group = 0;
    HSteamListenSocket listen_socket = 0;
    ostd::map<natural_t, fragment_accumulator> assembly;

    STEAM_CALLBACK(steam_transport_provider, on_status_changed, SteamNetConnectionStatusChangedCallback_t);
  };

}  // namespace other

#endif  // OTHER_STEAM_STEAM_STEAM_TRANSPORT_PROVIDER_HPP
