/**
 * \file driver/environment_registry.hpp
 **/
#ifndef OTHERLIB_DRIVER_ENVIRONMENT_REGISTRY_HPP
#define OTHERLIB_DRIVER_ENVIRONMENT_REGISTRY_HPP

#include <ranges>
#include <utility>

#include "core/enum_formatter.hpp"
#include "core/interfaces.hpp"
#include "core/scope.hpp"

#include "plugin/plugin_manifest.hpp"

namespace other {

  enum class interface_cardinality {
    SINGLE = 0,
    MULTIPLE,
    UNLIMITED,
  };

  enum class interface_scope {
    GLOBAL = 0,  // used for dynamic drivers
    DRIVER,
    PROJECT,

    NUM_INTERFACE_SCOPES,
    INVALID_INTERFACE_SCOPE = NUM_INTERFACE_SCOPES,
  };
  constexpr inline size_t kNumInterfaceScopes = static_cast<size_t>(interface_scope::NUM_INTERFACE_SCOPES);

  inline auto no_args() {
    return []() {
      return std::tuple<>();
    };
  }

  class OTHER_CLASS environment_registry {
   public:
    environment_registry(interface_scope scope)
        : registry_scope(scope) {}
    ~environment_registry() = default;

    template <typename T, typename Args>
      requires kIsEnvironmentInterface<T> && std::invocable<Args> &&
      std::convertible_to<std::invoke_result_t<Args>, typename T::construction_args_t>
    void declare_interface(
      std::function<natural_t(scope<T>)> on_provided, std::function<void(natural_t)> on_revoked,
      Args&& args_producer, interface_cardinality card = interface_cardinality::MULTIPLE
    );

    template <typename T, typename Args>
      requires kIsEnvironmentInterface<T> && std::invocable<Args> &&
      std::convertible_to<std::invoke_result_t<Args>, typename T::construction_args_t>
    void declare_interface(
      std::function<natural_t(scope<T>, plugin_param_view)> on_provided, std::function<void(natural_t)> on_revoked,
      Args&& args_producer, interface_cardinality card = interface_cardinality::MULTIPLE
    );

    natural_t install_from_manifest(std::string_view plugin_name, const plugin_manifest& m);
    void revoke_all_from(std::string_view plugin_name);

   private:
    struct registered_interface {
      natural_t interface_hash;
      std::string interface_full_name;
      interface_cardinality cardinality;

      std::function<void(natural_t)> revoke_thunk;
      std::function<natural_t(void*, plugin_param_view)> install_thunk;

      std::vector<natural_t> providers;

      inline std::vector<std::string> get_name_components() const {
        return interface_full_name |
          std::views::split('.') |
          std::views::transform([](auto&& part) { return std::string(part.begin(), part.end()); }) |
          std::ranges::to<std::vector<std::string>>();
      }
    };

    interface_scope registry_scope;
    std::vector<registered_interface> registered_interfaces;
  };

  template <typename T, typename Args>
    requires kIsEnvironmentInterface<T> && std::invocable<Args> &&
    std::convertible_to<std::invoke_result_t<Args>, typename T::construction_args_t>
  void environment_registry::declare_interface(std::function<natural_t(scope<T>)> on_provided, std::function<void(natural_t)> on_revoked, Args&& args_producer, interface_cardinality card) {
    if (std::ranges::find(registered_interfaces, T::kInterfaceHash, &registered_interface::interface_hash) != registered_interfaces.end()) {
      CORE_LOG_ERROR("Interface {} is already declared in environment registry for scope {}.", T::kFullInterfaceName, static_cast<uint32_t>(registry_scope));
      return;
    }

    CORE_LOG_DEBUG("Interface [{}] registered", T::kFullInterfaceName);
    CORE_LOG_DEBUG(" - Hash: {}", T::kInterfaceHash);
    CORE_LOG_DEBUG(" - Cardinality: {}", card);

    registered_interfaces.emplace_back(registered_interface{
      .interface_hash = T::kInterfaceHash,
      .interface_full_name = std::string(T::kFullInterfaceName),
      .cardinality = card,
      .revoke_thunk = std::move(on_revoked),
      .install_thunk = [install_fn = std::move(on_provided), args_producer = std::move(args_producer)](void* factory_address, plugin_param_view) -> natural_t {
        using factory_fn_t = T* (*)(void*);
        using args_t = typename T::construction_args_t;
        args_t args = args_producer();
        T* inst = reinterpret_cast<factory_fn_t>(factory_address)(reinterpret_cast<void*>(&args));
        OTHER_ASSERT(inst != nullptr, "Failed to create instance of interface {} using provided factory.", T::kFullInterfaceName);
        return install_fn(scope<T>{ inst });
      },
    });
  }

  template <typename T, typename Args>
    requires kIsEnvironmentInterface<T> && std::invocable<Args> &&
    std::convertible_to<std::invoke_result_t<Args>, typename T::construction_args_t>
  void environment_registry::declare_interface(std::function<natural_t(scope<T>, plugin_param_view)> on_provided, std::function<void(natural_t)> on_revoked, Args&& args_producer, interface_cardinality card) {
    if (std::ranges::find(registered_interfaces, T::kInterfaceHash, &registered_interface::interface_hash) != registered_interfaces.end()) {
      CORE_LOG_ERROR("Interface {} is already declared in environment registry for scope {}.", T::kFullInterfaceName, static_cast<uint32_t>(registry_scope));
      return;
    }

    CORE_LOG_DEBUG("Interface [{}] registered", T::kFullInterfaceName);
    CORE_LOG_DEBUG(" - Hash: {}", T::kInterfaceHash);
    CORE_LOG_DEBUG(" - Cardinality: {}", card);

    registered_interfaces.emplace_back(registered_interface{
      .interface_hash = T::kInterfaceHash,
      .interface_full_name = std::string(T::kFullInterfaceName),
      .cardinality = card,
      .revoke_thunk = std::move(on_revoked),
      .install_thunk = [install_fn = std::move(on_provided), args_producer = std::move(args_producer)](void* factory_address, plugin_param_view view) -> natural_t {
        using factory_fn_t = T* (*)(void*);
        using args_t = typename T::construction_args_t;
        args_t args = args_producer();
        T* inst = reinterpret_cast<factory_fn_t>(factory_address)(reinterpret_cast<void*>(&args));
        OTHER_ASSERT(inst != nullptr, "Failed to create instance of interface {} using provided factory.", T::kFullInterfaceName);
        return install_fn(scope<T>{ inst }, view);
      },
    });
  }

}  // namespace other

#endif  // OTHERLIB_DRIVER_ENVIRONMENT_REGISTRY_HPP