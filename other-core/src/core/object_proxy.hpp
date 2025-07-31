/**
 * \file core/object_proxy.hpp
 **/
#ifndef OTHER_CORE_OBJECT_PROXY_HPP
#define OTHER_CORE_OBJECT_PROXY_HPP

#include <type_traits>

#include <refl/refl.hpp>

#include "serialization/reflection.hpp"


namespace other {

  template <typename T>
  class object_proxy : public refl::runtime::proxy<object_proxy<T>, T> {
   public:
    object_proxy(T* object)
        : object(*object) {}

    object_proxy clone() const {
      return object_proxy(&object);
    }

    std::string object_type_name() const {
      return (std::string)refl::descriptor::get_name(descriptor);
    }

    template <typename... Args>
    void invoke_method(const std::string_view method_name, Args&&... args) {
      try {
        std::string mname{ method_name };

        if (sizeof...(Args) == 0) {
          refl::runtime::invoke<void, T>(std::forward<T>(object), mname.c_str());
        } else {
          refl::runtime::invoke<void, T>(std::forward<T>(object), mname.c_str(), std::forward<Args>(args)...);
        }
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception caught in invoke_method: {}", e.what());
      }
    }

    template <typename R, typename... Args>
    R invoke_method(const std::string_view method_name, Args&&... args) {
      try {
        std::string mname{ method_name };

        if (sizeof...(Args) == 0) {
          return refl::runtime::invoke<R, T>(std::forward<T>(object), mname.c_str());
        } else {
          return refl::runtime::invoke<R, T>(std::forward<T>(object), mname.c_str(), std::forward<Args>(args)...);
        }
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception caught in invoke_method: {}", e.what());
        return default_return<R>();
      }
    }

    template <typename Member, typename Self, typename... Args>
    static constexpr decltype(auto) invoke_impl(Self&& self, Args&&... args) {
      constexpr Member member{};
      if constexpr (refl::descriptor::is_field(member)) {
        static_assert(sizeof...(Args) <= 1, "Invalid number of arguments provided for property!");

        if constexpr (sizeof...(Args) == 1) {
          static_assert(refl::descriptor::is_writable(member));
          return member(self.target, std::forward<Args>(args)...);
        } else {
          static_assert(refl::descriptor::is_readable(member));
          return refl::util::make_const(member(self.target()));
        }
      } else {
        return member(self.target, std::forward<Args>(args)...);
      }
    }

   private:
    T& object;
    refl::descriptor::type_descriptor<T> descriptor{};

    template <typename R>
    static constexpr R default_return() {
      if constexpr (std::is_void_v<R>) {
        return;
      } else if constexpr (std::is_integral_v<R>) {
        return 0;
      } else if constexpr (std::is_floating_point_v<R>) {
        return 0.0;
      } else if constexpr (std::is_pointer_v<R>) {
        return nullptr;
      } else if constexpr (std::is_default_constructible_v<R>) {
        return R{};
      } else {
        static_assert(std::is_default_constructible_v<R>, "Type R must be default constructible or void");
      }
    }
  };

}  // namespace other

#endif  // OTHER_CORE_OBJECT_PROXY_HPP