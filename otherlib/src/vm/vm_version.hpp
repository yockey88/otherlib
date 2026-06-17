/**
 * \file vm/vm_version.hpp
 **/
#ifndef OTHERLIB_VM_VM_VERSION_HPP
#define OTHERLIB_VM_VM_VERSION_HPP

namespace other {

  struct vm_version {
    uint16_t major = 0;
    uint16_t minor = 0;
    uint32_t patch = 0;

    vm_version() = default;
    vm_version(uint16_t major, uint16_t minor, uint32_t patch)
        : major(major), minor(minor), patch(patch) {}
    constexpr bool operator>(const vm_version& other) const {
      return std::tie(major, minor, patch) > std::tie(other.major, other.minor, other.patch);
    }
    constexpr bool operator<(const vm_version& other) const {
      return std::tie(major, minor, patch) < std::tie(other.major, other.minor, other.patch);
    }
    constexpr bool operator==(const vm_version& other) const {
      return std::tie(major, minor, patch) == std::tie(other.major, other.minor, other.patch);
    }
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::vm_version> : formatter<string_view> {
    template <typename FormatContext>
    auto format(const other::vm_version& v, FormatContext& ctx) const {
      return format_to(ctx.out(), "{}.{}.{}", v.major, v.minor, v.patch);
    }
  };

}  // namespace std

#endif  // OTHERLIB_VM_VM_VERSION_HPP