/**
 * \file network/session_protocol_handler.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP
#define OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP

#include <cstdint>

#include "core/defines.hpp"
#include "core/version.hpp"
#include "thread/message.hpp"

namespace other {

  class session;

  enum class session_protocol {
    SESSION_SERVER_CHECK_IN,
    SESSION_CLIENT_CHECK_IN,
    SESSION_SHUTDOWN,
  };

#pragma pack(push, 1)

  struct handshake_header {
    uint8_t magic[4];
    version version_info;
  };

  struct session_handshake_data {
    handshake_header header = {
      { 'O', 'T', 'H', 'R' },
      {
        OTHERENV_VERSION_MAJOR,
        OTHERENV_VERSION_MINOR,
        OTHERENV_VERSION_PATCH,
      }
    };
    version min_version_supported = {
      OTHERENV_VERSION_MAJOR,
      OTHERENV_VERSION_MINOR,
      OTHERENV_VERSION_PATCH,
    };

    uint32_t flags = 0x00;
    enum protocol_flags : uint32_t {
      NONE = 0x00,
    };
  };

  struct protocol_handshake_data {
    handshake_header header;
    session_protocol protocol_id;
    uint32_t flags = 0x00;
  };

  // struct protocol_handshake_response {
  //   uint8_t magic[4];
  //   uint16_t protocol_version;
  //   uint16_t protocol_id;
  //   uint16_t min_version_supported;

  //   uint32_t flags;
  // };
#pragma pack(pop)

  struct message_sequence {
    struct message {
      enum : int8_t {
        NONE = -1,
        RX = 0,
        TX = 1,
      };
      int8_t rx_tx = NONE;
      message_header header;
      natural_t count;
    };

    struct sequence_step {
      enum : int8_t {
        SEQUENCE_START,
        SEQUENCE_END,
        RX_TX,

        CHOICE,
      };
      int8_t type;
      union {
        struct {
          natural_t choice_count;
          natural_t* choice_indices;
        };
        struct {
          natural_t sequence_index;
          int8_t rx_tx;
        };
      };
    };

    std::vector<sequence_step> steps;
    std::vector<message> messages;
  };

  struct session_protocol_data {
    session_protocol function;
    std::vector<message_sequence> sequences;
  };

  class protocol_handler {
   public:
    protocol_handler(session* s, const session_protocol_data& data)
        : protocol_data(data), session_ptr(s) {}
    virtual ~protocol_handler() = default;

    session_protocol get_protocol_id() const {
      return protocol_data.function;
    }

    void begin_protocol();
    virtual void on_protocol_start() {}
    void force_set_sequence_index(natural_t index, natural_t message_idx);

    bool poll();

   protected:
    message_sequence invert_message_sequence(const message_sequence& seq);

    bool current_msg_matches(const message_header& header);
    bool current_msg_matches(uint16_t category, uint16_t id) {
      return current_msg_matches(message_header{ .category = category, .id = id });
    }

    inline uint8_t get_ack_byte() const {
      return protocol_flags.should_nack ? 0x00 : 0x01;
    }
    inline void toggle_acknowledgement_flag() {
      protocol_flags.should_nack = !protocol_flags.should_nack;
    }

    session& get_session();

    message_sequence& get_current_sequence();
    message_sequence::message& get_current_message();

    bool receive_current_message();

    void complete_protocol();
    virtual void on_protocol_completion() {}

    virtual void transmit_current_message() = 0;

    virtual void handle_acknowledgement_ack(const message_header& header, const std::span<uint8_t> data) {}

    virtual void handle_control_ping(const message_header& header, const std::span<uint8_t> data) {}
    virtual void handle_control_pong(const message_header& header, const std::span<uint8_t> data) {}
    virtual void handle_control_version_handshake(const message_header& header, const std::span<uint8_t> data) {}

    virtual void handle_request_session_check_in(const message_header& header, const std::span<uint8_t> data) {}

    session_protocol_data protocol_data{};

   private:
    session* session_ptr = nullptr;

    natural_t sequence_index = 0;
    natural_t message_index = 0;
    natural_t message_count_index = 0;

    struct {
      bool should_nack = false;
    } protocol_flags{};

    void process_message(message&& msg);
    void increment_sequence_index();
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_SESSION_MANAGEMENT_FUNCTION_HPP