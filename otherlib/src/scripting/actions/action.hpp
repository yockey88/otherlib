/**
 * \file scripting/actions/action.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP

#include <string>
#include <type_traits>

#include "scripting/actions/callback.hpp"

namespace other {

  struct action {
    std::string name;
    std::string description;

    action() = default;

    template <typename R, typename... Args>
    action(std::function<R(Args...)> callback)
        : callback_fn(make_ref<native_callback<R, Args...>>(callback)) {}
    action(const std::string_view name, const std::string_view description)
        : name(name), description(description) {}
    action(const std::string_view name, const std::string_view description, ref<callback> cb)
        : name(name), description(description), callback_fn(std::move(cb)) {}
    virtual ~action() = default;

    void set_callback(ref<callback> cb);
    bool has_callback() const;

    template <typename R = void, typename... Args>
    R execute(Args&&... args) {
      if (callback_fn) {
        try {
          return callback_fn->template call<R, Args...>(std::forward<Args>(args)...);
        } catch (const callback_error& e) {
          CORE_LOG_ERROR("Error executing action '{}': {}", name, e.what());
          return default_return<R>();
        } catch (const std::exception& e) {
          CORE_LOG_ERROR("Standard exception executing action '{}': {}", name, e.what());
          return default_return<R>();
        } catch (...) {
          CORE_LOG_ERROR("Unknown error executing action '{}'", name);
          return default_return<R>();
        }
      } else {
        return default_return<R>();
      }
    }

    template <typename R>
    static R default_return() {
      if constexpr (std::is_same_v<R, void>) {
        return;
      } else {
        return R{};
      }
    }

   private:
    ref<callback> callback_fn;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP