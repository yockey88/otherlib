/**
 * \file dotnet/types.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_TYPES_HPP
#define OTHER_SCRIPTING_DOTNET_TYPES_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "serialization/reflection.hpp"

#include "dotnet/native_string.hpp"

namespace other {

  using nbool32 = uint32_t;

  enum managed_type {
    UNKNOWN_TYPE = 0,

    SBYTE_TYPE,
    BYTE_TYPE,
    SHORT_TYPE,
    USHORT_TYPE,
    INT_TYPE,
    UINT_TYPE,
    LONG_TYPE,
    ULONG_TYPE,

    FLOAT_TYPE,
    DOUBLE_TYPE,

    BOOL_TYPE,

    POINTER_TYPE,
    STRING_TYPE,
  };

  enum type_accessibility {
    PUBLIC_ACCESS,
    PRIVATE_ACCESS,
    PROTECTED_ACCESS,
    INTERNAL_ACCESS,
    PROTECTED_PUBLIC_ACCESS,
    PRIVATE_PROTECTED_ACCESS
  };

  namespace detail {

    template <typename TArg>
    constexpr managed_type get_managed_type() {
      /// callers forward arguments through tuples/std::apply (see dotnet_callback), so TArg
      ///  arrives as a (possibly const) lvalue reference — classify the underlying value type
      using T = std::remove_cvref_t<TArg>;
      if constexpr (std::is_pointer_v<T>) {
        return managed_type::POINTER_TYPE;
      } else if constexpr (std::same_as<T, uint8_t> || std::same_as<T, std::byte>) {
        return managed_type::BYTE_TYPE;
      } else if constexpr (std::same_as<T, uint16_t>) {
        return managed_type::USHORT_TYPE;
      } else if constexpr (std::same_as<T, uint32_t> || (std::same_as<T, unsigned long> && sizeof(T) == 4)) {
        return managed_type::UINT_TYPE;
      } else if constexpr (std::same_as<T, uint64_t> || (std::same_as<T, unsigned long> && sizeof(T) == 8)) {
        return managed_type::ULONG_TYPE;
      } else if constexpr (std::same_as<T, char8_t>) {
        return managed_type::SBYTE_TYPE;
      } else if constexpr (std::same_as<T, int16_t>) {
        return managed_type::SHORT_TYPE;
      } else if constexpr (std::same_as<T, int32_t> || (std::same_as<T, long> && sizeof(T) == 4)) {
        return managed_type::INT_TYPE;
      } else if constexpr (std::same_as<T, int64_t> || (std::same_as<T, long> && sizeof(T) == 8)) {
        return managed_type::LONG_TYPE;
      } else if constexpr (std::same_as<T, float>) {
        return managed_type::FLOAT_TYPE;
      } else if constexpr (std::same_as<T, double>) {
        return managed_type::DOUBLE_TYPE;
      } else if constexpr (std::same_as<T, bool>) {
        return managed_type::BOOL_TYPE;
      } else if constexpr (std::is_same_v<T, native_string>) {
        return managed_type::STRING_TYPE;
      } else {
        return managed_type::UNKNOWN_TYPE;
      }
    }

    template <typename T>
    concept dotnet_stringlike = std::same_as<T, native_string>;

    template <typename A, size_t I>
    inline void add_to_array_at_index(const void** args_arr, managed_type* param_types, A&& in_arg) {
      param_types[I] = get_managed_type<A>();
      if constexpr (std::is_pointer_v<std::remove_reference_t<A>>) {
        args_arr[I] = reinterpret_cast<const void*>(in_arg);
      } else {
        args_arr[I] = reinterpret_cast<const void*>(&in_arg);
      }
    }

    template <typename... Args, size_t... Is>
    inline void create_opaque_handle_array(const void** args, managed_type* parameters, Args&&... values, const std::index_sequence<Is...>&) {
      (add_to_array_at_index<Args, Is>(args, parameters, std::forward<Args>(values)), ...);
    }

  }  // namespace detail

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_TYPES_HPP