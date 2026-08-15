/**
 * \file network/link_security.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_LINK_SECURITY_HPP
#define OTHER_NETWORK_NETWORK_LINK_SECURITY_HPP

#include <span>

#include "core/defines.hpp"
#include "core/interfaces.hpp"

#include "network/link.hpp"

namespace other {

  class link_sink;

  /// configurable per-mesh security layer; none installed = zero cost, no AUTHENTICATING state.
  ///  control frames are exempt from encrypt/decrypt — keepalive must survive a misconfigured key
  class OTHER_CLASS link_security {
    OTHER_ENVIRONMENT_INTERFACE("Network", "LinkSecurity");

   public:
    enum class auth_result : uint8_t {
      PENDING = 0,
      ESTABLISHED = 1,
      FAILED = 2,
    };

    link_security() = default;
    virtual ~link_security() = default;

    virtual std::string_view name() const = 0;

    /// called when both hellos validate; may emit LINK_AUTH frames via
    ///  link_sink::send_auth. FAILED closes the link
    virtual auth_result begin_auth(link_sink& link, link_record& record) = 0;
    /// called per received LINK_AUTH frame while AUTHENTICATING
    virtual auth_result on_auth_frame(link_sink& link, link_record& record, std::span<const uint8_t> payload) = 0;

    /// per-frame payload transforms on UP links; identity by default. encrypt runs
    ///  last on tx (after filters), decrypt first on rx (before anything reads payload)
    virtual bool encrypt(const link_record& record, ostd::vector<uint8_t>& payload) { return true; }
    virtual bool decrypt(const link_record& record, std::span<uint8_t> payload) { return true; }
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_LINK_SECURITY_HPP
