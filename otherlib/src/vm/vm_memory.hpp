/**
 * \file vm/vm_memory.hpp
 **/
#ifndef OTHERLIB_VM_VM_MEMORY_HPP
#define OTHERLIB_VM_VM_MEMORY_HPP

#include <algorithm>
#include <format>
#include <span>

namespace other {

  template <size_t N>
    requires(N % 2 == 0 && N > 0)
  struct register_memory_t {
    alignas(1) uint8_t data[N / 8];

    void* memory() { return static_cast<void*>(data); }
    const void* memory() const { return static_cast<const void*>(data); }

    bool is_zero() const {
      return std::all_of(std::begin(data), std::end(data), [](uint8_t byte) { return byte == 0; });
    }
    uint64_t to_u64() const {
      static_assert(N <= 64, "Cannot convert register larger than 64 bits to uint64_t");
      uint64_t value = 0;
      for (size_t i = 0; i < N / 8; ++i) {
        value |= static_cast<uint64_t>(data[i]) << (i * 8);
      }
      return value;
    }

    register_memory_t() : data{} { std::ranges::fill(std::span(data, N / 8), 0); }
    template <typename I>
      requires std::is_integral_v<I>
    register_memory_t(I value) {
      static_assert(sizeof(I) <= N / 8, "Integral type too large to fit in register memory");
      const void* value_ptr = static_cast<const void*>(&value);
      const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(value_ptr);
      auto bytes = std::span(value_bytes, sizeof(I));
      std::ranges::copy(bytes, data);
    }
  };
  static_assert(sizeof(register_memory_t<8>) == sizeof(uint8_t), "Register memory size incorrect");
  static_assert(sizeof(register_memory_t<16>) == sizeof(uint16_t), "Register memory size incorrect");
  static_assert(sizeof(register_memory_t<32>) == sizeof(uint32_t), "Register memory size incorrect");
  static_assert(sizeof(register_memory_t<64>) == sizeof(uint64_t), "Register memory size incorrect");

}  // namespace other

namespace std {

  template <>
  struct formatter<other::register_memory_t<64>> : public formatter<std::string_view> {
    auto format(const other::register_memory_t<64>& reg, format_context& ctx) const {
      return formatter<std::string_view>::format(std::format("{:#018x}", reg.to_u64()), ctx);
    }
  };

}  // namespace std

namespace other {

  template <size_t N>
  using register_t = register_memory_t<N>;

  template <size_t N>
  struct memory_storage_t {
    constexpr static size_t size() { return N; }
    uint8_t data[N];

    constexpr memory_storage_t() : data{} {
      std::ranges::fill(std::span(data, N), 0);
    }

    uint8_t& at(const size_t index) { return *checked_get_ptr_at(index); }
    const uint8_t& at(const size_t index) const { return *checked_get_ptr_at(index); }
    uint8_t* checked_get_ptr_at(const size_t index) {
      assert(index < N && "Index out of bounds");
      return &data[index];
    }

    void write_byte(const size_t index, const uint8_t value) {
      assert(index < size() && "Index out of bounds");
      data[index] = value;
    }
    uint8_t read_byte(const size_t index) const {
      assert(index < size() && "Index out of bounds");
      return data[index];
    }

    void* start() { return data; }
    void* end() { return data + N; }
    void* unsafe_at(const size_t index) { return data + index; }

    void* ptr_to(const size_t index) {
      assert(index < N && "Index out of bounds");
      return unsafe_at(index);
    }
  };
  /// just to make sure
  static_assert(sizeof(memory_storage_t<8>) == 8, "Memory storage size incorrect");
  static_assert(sizeof(memory_storage_t<16>) == 16, "Memory storage size incorrect");
  static_assert(sizeof(memory_storage_t<1024>) == 1024, "Memory storage size incorrect");

}  // namespace other

#endif  // OTHERLIB_VM_VM_MEMORY_HPP