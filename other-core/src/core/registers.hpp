/**
 * \file core/registers.hpp
 **/
#ifndef OTHER_CORE_REGISTERS_HPP
#define OTHER_CORE_REGISTERS_HPP

#include <cstdint>
#include <vector>

#include "core/memory_pool.hpp"
#include "core/value.hpp"

namespace other {

#pragma pack(push, 1)
  struct address_t {
    uint8_t segment = 0;
    uint32_t index = 0;

    address_t() = default;
    constexpr address_t(natural_t addr) {
      segment = static_cast<uint8_t>(addr >> 32);
      index = static_cast<uint32_t>(addr & 0xFFFFFFFF);
    }
    constexpr address_t(uint8_t segment, uint32_t index)
        : segment(segment), index(index) {}

    constexpr auto operator<=>(const address_t&) const = default;

    operator natural_t() const {
      return (static_cast<natural_t>(segment) << 32) | index;
    }
  };
#pragma pack(pop)

  namespace address {
    static constexpr address_t kNullAddress = { 0u, 0u };
  }

  struct value_reference {
    value_reference(value& value);
    ~value_reference();

    value& val;
  };

  class registers {
   public:
   private:
  };

}  // namespace other

#endif  // OTHER_CORE_REGISTERS_HPP
