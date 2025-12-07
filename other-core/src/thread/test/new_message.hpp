/**
 * \file thread/test/new_message.hpp
 **/
#ifndef OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP
#define OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP

#include <span>
#include <vector>

namespace other {

  struct other_message_spec {
    virtual ~other_message_spec() = default;

    std::span<const uint8_t> as_buffer();

    template <typename T>
    static T parse(const std::span<const uint8_t> data);

   protected:
    uint8_t* get_data_ptr();
    void initialize_data_ptr(const uint8_t* ptr, size_t size);

    virtual void on_initialize_data() = 0;

   private:
    const uint8_t* data_ptr = nullptr;
    size_t data_size = 0;
  };

  template <typename T>
  struct other_message_spec_impl : other_message_spec {
    virtual ~other_message_spec_impl() = default;

    void on_initialize_data() override {
      if constexpr (requires { T::custom_builder(std::declval<T*>()); }) {
        std::vector<uint8_t> built_data = T::custom_builder(reinterpret_cast<T*>(this));
        initialize_data_ptr(built_data.data(), built_data.size());
      } else {
        const T* self = reinterpret_cast<const T*>(this);
        initialize_data_ptr(reinterpret_cast<const uint8_t*>(self), sizeof(T));
      }
    }

    static decltype(auto) from_buffer(const std::span<const uint8_t> data) {
      if constexpr (requires { T::custom_parser(std::declval<const std::span<const uint8_t>>()); }) {
        return T::custom_parser(data);
      } else {
        OTHER_ASSERT(data.size() >= sizeof(T), "Invalid data size for message parsing!");
        T msg;
        std::memcpy(&msg, data.data(), sizeof(T));
        return msg;
      }
    }
  };

  template <typename T>
  T other_message_spec::parse(const std::span<const uint8_t> data) {
    return other_message_spec_impl<T>::from_buffer(data);
  }

}  // namespace other

#endif  // OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP