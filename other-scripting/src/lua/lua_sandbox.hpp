/**
 * \file lua/lua_sandbox.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_LUA_LUA_SANDBOX_HPP
#define OTHERLIB_SCRIPTING_LUA_LUA_SANDBOX_HPP

#include <sol/sol.hpp>

namespace other {

  class lua_sandbox {
   public:
    lua_sandbox(sol::state& lua_state)
        : sandbox_environment(lua_state, sol::create, lua_state.globals()) {}
    lua_sandbox(sol::environment&& env)
        : sandbox_environment(std::move(env)) {}

    template <typename T>
    auto operator[](T&& key) & {
      return sandbox_environment.template operator[](std::forward<T>(key));
    }

    template <typename T>
    auto operator[](T&& key) const& {
      return sandbox_environment.template operator[](std::forward<T>(key));
    }

    template <typename T>
    auto operator[](T&& key) && {
      return std::move(sandbox_environment).template operator[](std::forward<T>(key));
    }

    sol::environment& environment() { return sandbox_environment; }
    const sol::environment& environment() const { return sandbox_environment; }

   private:
    sol::environment sandbox_environment;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_LUA_LUA_SANDBOX_HPP