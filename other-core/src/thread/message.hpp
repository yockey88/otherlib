/**
 * \file thread/message.hpp
 **/
#ifndef OTHERLIB_THREAD_MESSAGE_HPP
#define OTHERLIB_THREAD_MESSAGE_HPP

#include <cstdint>
#include <string>
#include <vector>

#define ASIO_HAS_STD_INVOKE_RESULT
#include <asio/asio.hpp>

#include "thread/channel.hpp"

namespace other {

  enum message_category : uint8_t {
    NOTIFICATION = 0,

    CONTROL,

    COMMAND,
    QUERY,
    RESPONSE,

    ACKNOWLEDGEMENT,
    ERROR_ALERT,

    INFO,

    SIMULATION,

    SIMULATION_EVENT,
  };

  enum message_id : uint8_t {
    NACK = 0x00,
    ACK = 0x01,

    PING = 0x02,
    PONG = 0x03,

    SIM_DESCRIPTION,

    SIM_EVENT,

    NODE_INITIALIZATION_REQUEST,
    NODE_SHUTDOWN_REQUEST,

    /// thread messages
    THREAD_INITIALIZE,
    THREAD_START,
    THREAD_SHUTDOWN,
  };

  struct message_header {
    message_category category;
    message_id id;
  };

  struct binding_point {
    uint16_t port = 0;
    union {
      uint32_t ip;
      uint8_t bytes[4] = { 0, 0, 0, 0 };
    };

    static std::string write_string(const binding_point& bp);
    static binding_point from_asio(const asio::ip::address& addr, uint16_t port);
  };

  struct session_endpoint {
    /// taps all nodes in simulation, receiving tagged packets
    ///  to monitor total network traffic
    binding_point simulation;

    /// generic control endpoint
    binding_point control;

    static std::string write_string(const session_endpoint& endpoint);
  };

  struct message;

  template <typename T>
  concept message_spec_type = requires(T t) {
    { T::category } -> std::same_as<message_category>;
    { T::id } -> std::same_as<message_id>;
    { T::parse(std::declval<const std::vector<uint8_t>&>()) } -> std::same_as<T>;
    { t.build() } -> std::same_as<std::vector<uint8_t>>;
  };

  template <typename T, typename... Args>
  concept self_building_message = std::constructible_from<T, Args...> && requires(const T& obj, Args&&... args) {
    { obj.build() } -> std::same_as<message>;
  };

  template <typename T>
  concept self_parsing_message = requires(const T& obj) {
    { obj.parse(std::declval<const std::vector<uint8_t>&>()) } -> std::same_as<T>;
  };

  template <typename T>
  struct message_spec_impl;

  struct message_spec {
    virtual ~message_spec() = default;
    virtual std::vector<uint8_t> build_message() = 0;

   protected:
    void write_header(std::vector<uint8_t>& data, const message_header& header);

    // std::vector<uint8_t> build_message()
  };

  template <typename T>
  struct message_spec_impl : message_spec {
    std::vector<uint8_t> build_message() override;
  };

  struct message {
    message_header header;
    std::vector<uint8_t> data;

    void set_category(message_category category) { header.category = category; }
    message_category get_category() const { return header.category; }

    void set_id(message_id id) { header.id = id; }
    message_id get_id() const { return header.id; }

    message() = default;
    message(message_category category, message_id type) {
      header = { category, type };
    }

    message(const message_header& msg_header, const std::vector<uint8_t>& msg_data)
        : header(msg_header), data(msg_data) {}

    template <typename T>
      requires self_parsing_message<T>
    T parse_message(const std::vector<uint8_t>& data) const {
      return T::parse(data);
    }
  };

  template <typename T>
  std::vector<uint8_t> message_spec_impl<T>::build_message() {
    return reinterpret_cast<T*>(this)->build();
  }

  using message_channel = channel<message>;

  struct field_bounds {
    natural_t min = 0;
    natural_t max = 0;
  };
  struct message_field {
    const char* name;
    field_bounds size = { 0, 0 };
  };

  constexpr inline message_field message_fields[] = {
    { "msg-category", { sizeof(message_category), sizeof(message_category) } },
    { "msg-id", { sizeof(message_id), sizeof(message_id) } },

    { "session-type", { sizeof(uint16_t), sizeof(uint16_t) } },
    { "node-id", { sizeof(uint64_t), sizeof(uint64_t) } },
    { "layer-type", { 0, sizeof(uint8_t) } },
    { "status", { sizeof(uint64_t), sizeof(uint64_t) } },

    { "acked-header", { sizeof(message_header), sizeof(message_header) } },
    { "ack-nack", { sizeof(uint8_t), sizeof(uint8_t) } },

    { "port", { sizeof(uint16_t), sizeof(uint16_t) } },
    { "ip", { sizeof(uint32_t), sizeof(uint32_t) } },

    { "simulation-binding-point", { sizeof(binding_point), sizeof(binding_point) } },
    { "comm-layer-binding-point", { sizeof(binding_point), sizeof(binding_point) } },
    { "analytic-layer-binding-point", { sizeof(binding_point), sizeof(binding_point) } },
  };

  enum message_field_idx : uint8_t {
    MSG_CATEGORY_FIELD = 0,
    MSG_ID_FIELD,

    SESSION_TYPE_FIELD,
    NODE_ID_FIELD,
    LAYER_TYPE_FIELD,
    STATUS_FIELD,

    ACKED_HEADER_FIELD,
    ACK_NACK_FIELD,

    PORT_FIELD,
    IP_FIELD,

    SIMULATION_BINDING_POINT_FIELD,
    COMM_LAYER_BINDING_POINT_FIELD,
    ANALYTIC_LAYER_BINDING_POINT_FIELD,
  };

  struct acknowledgement : message_spec_impl<acknowledgement> {
    constexpr static message_category category = ACKNOWLEDGEMENT;
    constexpr static message_id id = ACK;

    message_header acked_header;
    uint8_t ack_nack = 0;
    uint64_t node_id = 0;

    static acknowledgement parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const acknowledgement& msg);
  };

  struct session_status_request : message_spec_impl<session_status_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = PING;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint8_t layer_type = 0;

    static session_status_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_status_request& msg);
  };

  struct session_status_response : message_spec_impl<session_status_response> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = PONG;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    static session_status_response parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_status_response& msg);
  };

  struct simulation_description : message_spec_impl<simulation_description> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = SIM_DESCRIPTION;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    binding_point simulation_binding_point;

    static simulation_description parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const simulation_description& msg);
  };

  struct node_initialization_request : message_spec_impl<node_initialization_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = NODE_INITIALIZATION_REQUEST;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    binding_point comm_layer_endpoint;
    binding_point analytics_layer_endpoint;
    /// other settings related to bringin a node in the simulation up

    static node_initialization_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const node_initialization_request& msg);
  };

  struct session_shutdown_request : message_spec_impl<session_shutdown_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = NODE_SHUTDOWN_REQUEST;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    static session_shutdown_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_shutdown_request& msg);
  };

}  // namespace other

#endif  // OTHERLIB_THREAD_MESSAGE_HPP