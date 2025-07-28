/**
 * \file dotnet/types.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_TYPES_HPP
#define OTHER_SCRIPTING_DOTNET_TYPES_HPP

#include <cstdint>

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

  enum field_accessibility {
    PUBLIC_ACCESS,
    PRIVATE_ACCESS,
    PROTECTED_ACCESS,
    INTERNAL_ACCESS,
    PROTECTED_PUBLIC_ACCESS,
    PRIVATE_PROTECTED_ACCESS
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_TYPES_HPP