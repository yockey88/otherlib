/**
 * \file core/interfaces.hpp
 **/
#ifndef OTHER_CORE_CORE_INTERFACES_HPP
#define OTHER_CORE_CORE_INTERFACES_HPP

#include <string_view>

#include "core/fnv.hpp"

#define OTHER_ENVIRONMENT_INTERFACE(module_name, interface_name)                                    \
 public:                                                                                            \
  static constexpr ::std::string_view kPluginModuleName = module_name;                              \
  static constexpr ::std::string_view kInterfaceName = interface_name;                              \
  static constexpr ::std::string_view kFullInterfaceName = "Other." module_name "." interface_name; \
  static constexpr ::other::natural_t kInterfaceHash = ::other::FNV(kFullInterfaceName);

namespace other {

  template <typename T>
  concept is_environment_interface = requires {
    { T::kPluginModuleName } -> std::convertible_to<std::string_view>;
    { T::kInterfaceName } -> std::convertible_to<std::string_view>;
    { T::kFullInterfaceName } -> std::convertible_to<std::string_view>;
    { T::kInterfaceHash } -> std::convertible_to<other::natural_t>;
  };

  template <typename T>
  constexpr inline bool kIsEnvironmentInterface = is_environment_interface<T>;

}  // namespace other

#endif  // OTHER_CORE_CORE_INTERFACES_HPP