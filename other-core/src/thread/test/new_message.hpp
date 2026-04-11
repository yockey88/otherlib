/**
 * \file thread/test/new_message.hpp
 **/
#ifndef OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP
#define OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP

#include <span>

#include "serialization/serialization.hpp"

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
        std::vector<uint8_t> built_data = build_data();
        initialize_data_ptr(built_data.data(), built_data.size());
      }
    }

    static decltype(auto) from_buffer(const std::span<const uint8_t> data) {
      if constexpr (requires { T::custom_parser(std::declval<const std::span<const uint8_t>>()); }) {
        return T::custom_parser(data);
      } else {
        T msg = read_object(data);
        return msg;
      }
    }

   private:
    std::vector<uint8_t> build_data() {
      std::vector<uint8_t> data;

      const T* self = reinterpret_cast<const T*>(this);

      if constexpr (reflected_type<T>) {
        refl::util::for_each(refl::reflect(*self).members, [&](auto field) {
          std::string field_name = std::string{ field.name };
          auto val = field(*self);
          using field_t = std::remove_cvref_t<decltype(val)>;

          if constexpr (std::is_pointer_v<field_t>) {
            uintptr_t ptr_value = reinterpret_cast<uintptr_t>(val);
            const uint8_t* field_ptr = reinterpret_cast<const uint8_t*>(&ptr_value);
            data.append_range(std::span(field_ptr, sizeof(field_t)));
          } else if constexpr (std::is_trivially_copyable_v<field_t>) {
            const uint8_t* field_ptr = reinterpret_cast<const uint8_t*>(&(reinterpret_cast<const T*>(this)->*field.pointer));
            data.append_range(std::span(field_ptr, sizeof(field_t)));
          } else {
            // CORE_LOG_ERROR("[DEV-NOTICE]: Unimplemented field type in other_message_spec_impl::build_data for field '{}'", field_name);
          }
        });
      } else {
        // CORE_LOG_ERROR("[DEV-NOTICE]: Attempted to build data for non-reflected type in other_message_spec_impl::build_data");
      }

      return data;
    }

    static T read_object(std::span<const uint8_t> data) {
      T msg;
      if constexpr (reflected_type<T>) {
        refl::util::for_each(refl::reflect(msg).members, [&](auto field) {
          std::string field_name = std::string{ field.name };
          using field_t = std::remove_cvref_t<decltype(field(msg))>;

          if constexpr (std::is_pointer_v<field_t>) {
            OTHER_ASSERT(sizeof(uintptr_t) <= data.size(), "Insufficient data to read pointer field '{}' in other_message_spec_impl::read_object", field_name);
            uintptr_t ptr_value = *reinterpret_cast<const uintptr_t*>(data.subspan(0, sizeof(uintptr_t)).data());
            field(msg) = reinterpret_cast<typename std::remove_pointer<field_t>::type*>(ptr_value);
            data = data.subspan(sizeof(uintptr_t));
          } else if constexpr (std::is_trivially_copyable_v<field_t>) {
            OTHER_ASSERT(sizeof(field_t) <= data.size(), "Insufficient data to read field '{}' in other_message_spec_impl::read_object", field_name);
            field(msg) = *reinterpret_cast<const field_t*>(data.subspan(0, sizeof(field_t)).data());
            data = data.subspan(sizeof(field_t));
          } else {
            // CORE_LOG_ERROR("[DEV-NOTICE]: Unimplemented field type in other_message_spec_impl::read_object for field '{}'", field_name);
          }
        });
      } else if constexpr (std::is_trivially_copyable_v<T>) {
        OTHER_ASSERT(sizeof(T) <= data.size(), "Insufficient data to read object of type '{}' in other_message_spec_impl::read_object", typeid(T).name());
        msg = *reinterpret_cast<const T*>(data.subspan(0, sizeof(T)).data());
        data = data.subspan(sizeof(T));
        // CORE_LOG_ERROR("[DEV-NOTICE]: Attempted to read object for non-reflected type in other_message_spec_impl::read_object");
      }
      return msg;
    }
  };

  template <typename T>
  T other_message_spec::parse(const std::span<const uint8_t> data) {
    return other_message_spec_impl<T>::from_buffer(data);
  }

}  // namespace other

#endif  // OTHER_CORE_THREAD_TEST_NEW_MESSAGE_HPP