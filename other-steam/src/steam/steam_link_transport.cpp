/**
 * \file steam/steam_link_transport.cpp
 **/
#include "steam/steam_link_transport.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  ostd::vector<ostd::vector<uint8_t>> fragment_blob(std::span<const uint8_t> blob, size_t max_payload) {
    ostd::vector<ostd::vector<uint8_t>> chunks;
    uint16_t index = 0;
    size_t offset = 0;
    do {
      const size_t take = std::min(max_payload, blob.size() - offset);
      const bool final_chunk = offset + take == blob.size();
      ostd::vector<uint8_t> chunk(kSteamFragmentHeader + take);
      chunk[0] = final_chunk ? 1 : 0;
      chunk[1] = static_cast<uint8_t>(index & 0xFF);
      chunk[2] = static_cast<uint8_t>(index >> 8);
      std::ranges::copy(blob.subspan(offset, take), chunk.begin() + kSteamFragmentHeader);
      chunks.push_back(std::move(chunk));
      offset += take;
      index++;
    } while (offset < blob.size());
    return chunks;
  }

  opt<ostd::vector<uint8_t>> fragment_accumulator::feed(std::span<const uint8_t> chunk) {
    if (chunk.size() < kSteamFragmentHeader) {
      pending.clear();
      next_index = 0;
      return std::nullopt;
    }
    const bool final_chunk = (chunk[0] & 1) != 0;
    const uint16_t index = static_cast<uint16_t>(chunk[1] | (chunk[2] << 8));
    if (index != next_index) {
      pending.clear();
      next_index = 0;
      if (index != 0) {
        return std::nullopt;
      }
    }

    pending.append_range(chunk.subspan(kSteamFragmentHeader));
    if (!final_chunk) {
      next_index++;
      return std::nullopt;
    }
    ostd::vector<uint8_t> blob = std::move(pending);
    pending.clear();
    next_index = 0;
    return blob;
  }

  steam_link_transport::steam_link_transport(int virtual_port)
      : virtual_port(virtual_port) {
    ISteamNetworkingSockets* sockets = SteamNetworkingSockets();
    OTHER_ASSERT(sockets != nullptr, "steam_link_transport requires a READY steam context.");
    poll_group = sockets->CreatePollGroup();
  }

  steam_link_transport::~steam_link_transport() {
    if (ISteamNetworkingSockets* sockets = SteamNetworkingSockets(); sockets != nullptr) {
      if (listen_socket != 0) {
        sockets->CloseListenSocket(listen_socket);
      }
      if (poll_group != 0) {
        sockets->DestroyPollGroup(poll_group);
      }
    }
  }

  node_id steam_link_transport::attested_remote(natural_t conn_id) const {
    SteamNetConnectionInfo_t info{};
    if (!SteamNetworkingSockets()->GetConnectionInfo(static_cast<HSteamNetConnection>(conn_id), &info)) {
      return 0;
    }
    return info.m_identityRemote.GetSteamID64();
  }

  natural_t steam_link_transport::dial(const net_address& remote) {
    /// lobby establishment resolves to a peer id before any dial (glue's job) —
    ///  the transport only ever connects to an attested peer identity
    if (remote.addressing != net_address::kind::STEAM_PEER || remote.id == 0) {
      CORE_LOG_WARN("[STEAM] dial refused: not a steam peer address");
      return 0;
    }
    SteamNetworkingIdentity identity;
    identity.SetSteamID64(remote.id);
    const HSteamNetConnection conn = SteamNetworkingSockets()->ConnectP2P(identity, virtual_port, 0, nullptr);
    return conn == k_HSteamNetConnection_Invalid ? 0 : conn;
  }

  natural_t steam_link_transport::listen(const net_address& bind_addr) {
    if (listen_socket != 0) {
      CORE_LOG_WARN("[STEAM] listen refused: already listening");
      return 0;
    }
    listen_socket = SteamNetworkingSockets()->CreateListenSocketP2P(virtual_port, 0, nullptr);
    return listen_socket;
  }

  void steam_link_transport::tx(natural_t conn_id, std::span<const uint8_t> bytes) {
    PROFILE_SECTION("steam_link_transport::tx");
    ISteamNetworkingSockets* sockets = SteamNetworkingSockets();
    for (const ostd::vector<uint8_t>& chunk : fragment_blob(bytes)) {
      sockets->SendMessageToConnection(static_cast<HSteamNetConnection>(conn_id), chunk.data(),
                                       static_cast<uint32_t>(chunk.size()), k_nSteamNetworkingSend_Reliable, nullptr);
    }
  }

  void steam_link_transport::close(natural_t conn_id) {
    SteamNetworkingSockets()->CloseConnection(static_cast<HSteamNetConnection>(conn_id), 0, nullptr, false);
    assembly.erase(conn_id);
  }

  void steam_link_transport::pump() {
    if (poll_group == 0) {
      return;
    }
    PROFILE_SECTION("steam_link_transport::pump");
    SteamNetworkingMessage_t* messages[64] = {};
    const int count = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup(poll_group, messages, 64);
    for (int i = 0; i < count; ++i) {
      SteamNetworkingMessage_t* message = messages[i];
      const natural_t conn = message->m_conn;
      opt<ostd::vector<uint8_t>> blob = assembly[conn].feed(
        std::span<const uint8_t>(static_cast<const uint8_t*>(message->m_pData), static_cast<size_t>(message->m_cbSize)));
      message->Release();
      if (blob.has_value() && consumer.received != nullptr) {
        consumer.received(conn, *blob);
      }
    }
  }

  void steam_link_transport::on_status_changed(SteamNetConnectionStatusChangedCallback_t* status) {
    ISteamNetworkingSockets* sockets = SteamNetworkingSockets();
    const natural_t conn = status->m_hConn;

    switch (status->m_info.m_eState) {
      case k_ESteamNetworkingConnectionState_Connecting:
        /// transport accepts, protocol decides: session policy runs at JOIN_REQUEST
        if (listen_socket != 0 && status->m_info.m_hListenSocket == listen_socket) {
          if (sockets->AcceptConnection(status->m_hConn) != k_EResultOK) {
            sockets->CloseConnection(status->m_hConn, 0, nullptr, false);
          }
        }
        return;

      case k_ESteamNetworkingConnectionState_Connected:
        sockets->SetConnectionPollGroup(status->m_hConn, poll_group);
        if (status->m_info.m_hListenSocket != 0) {
          if (consumer.accepted != nullptr) {
            consumer.accepted(status->m_info.m_hListenSocket, conn);
          }
        } else if (consumer.opened != nullptr) {
          consumer.opened(conn);
        }
        return;

      case k_ESteamNetworkingConnectionState_ClosedByPeer:
      case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        CORE_LOG_TRACE("[STEAM] conn {} closed ({})", conn, status->m_info.m_szEndDebug);
        sockets->CloseConnection(status->m_hConn, 0, nullptr, false);
        assembly.erase(conn);
        if (consumer.closed != nullptr) {
          consumer.closed(conn);
        }
        return;

      default:
        return;  // None/FindingRoute/FinWait/Linger
    }
  }

}  // namespace other
