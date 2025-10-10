/**
 * \file dotnet/types.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_TYPES_HPP
#define OTHER_SCRIPTING_DOTNET_TYPES_HPP

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

#include "serialization/reflection.hpp"

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
      if constexpr (std::is_pointer_v<std::remove_reference_t<TArg>>) {
        return managed_type::POINTER_TYPE;
      } else if constexpr (std::same_as<TArg, uint8_t> || std::same_as<TArg, std::byte>) {
        return managed_type::BYTE_TYPE;
      } else if constexpr (std::same_as<TArg, uint16_t>) {
        return managed_type::USHORT_TYPE;
      } else if constexpr (std::same_as<TArg, uint32_t> || (std::same_as<TArg, unsigned long> && sizeof(TArg) == 4)) {
        return managed_type::UINT_TYPE;
      } else if constexpr (std::same_as<TArg, uint64_t> || (std::same_as<TArg, unsigned long> && sizeof(TArg) == 8)) {
        return managed_type::ULONG_TYPE;
      } else if constexpr (std::same_as<TArg, char8_t>) {
        return managed_type::SBYTE_TYPE;
      } else if constexpr (std::same_as<TArg, int16_t>) {
        return managed_type::SHORT_TYPE;
      } else if constexpr (std::same_as<TArg, int32_t> || (std::same_as<TArg, long> && sizeof(TArg) == 4)) {
        return managed_type::INT_TYPE;
      } else if constexpr (std::same_as<TArg, int64_t> || (std::same_as<TArg, long> && sizeof(TArg) == 8)) {
        return managed_type::LONG_TYPE;
      } else if constexpr (std::same_as<TArg, float>) {
        return managed_type::FLOAT_TYPE;
      } else if constexpr (std::same_as<TArg, double>) {
        return managed_type::DOUBLE_TYPE;
      } else if constexpr (std::same_as<TArg, bool>) {
        return managed_type::BOOL_TYPE;
      } else {
        return managed_type::UNKNOWN_TYPE;
      }
    }

    template <typename A, size_t I>
      requires(!is_stringlike_type<std::remove_cvref_t<A>>)
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