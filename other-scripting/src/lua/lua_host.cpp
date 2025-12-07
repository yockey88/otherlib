/**
 * \file lua/lua_host.cpp
 **/
#include "lua/lua_host.hpp"

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "lua/lua_script.hpp"

#include "sol/load_result.hpp"

namespace other {

  void lua_host::load_host(const config_table& config) {
    lua_state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::io, sol::lib::os, sol::lib::debug);
  }

  void lua_host::call_entry_point() {
  }

  lua_script* lua_host::load_file(const std::string_view file_path) {
    filepath fpath{ file_path };
    if (fpath.extension() != ".lua") {
      CORE_LOG_ERROR("Lua host can only load .lua files, got '{}'", fpath.extension().string());
      return nullptr;
    }

    auto hash = FNV(fpath.string());
    if (auto itr = loaded_lua_scripts.find(hash); itr != loaded_lua_scripts.end()) {
      return &itr->second;
    }

    auto [itr, success] = loaded_scripts.insert({ hash, lua_state.load_file(fpath.string()) });
    if (!success) {
      CORE_LOG_ERROR("Failed to load Lua script '{}'", fpath.string());
      return nullptr;
    }
    if (!itr->second.valid()) {
      sol::error err = itr->second;
      CORE_LOG_ERROR("Error loading Lua script '{}': {}", fpath.string(), err.what());
      loaded_scripts.erase(itr);
      return nullptr;
    }

    sol::environment env(lua_state, sol::create, lua_state.globals());
    env["__script_file_path"] = fpath.string();
    env["__script_file_name"] = fpath.filename().string();
    env["__script_name"] = fpath.stem().string();

    auto [script_itr, script_success] = loaded_lua_scripts.insert({ hash, lua_script(lua_state, std::move(env), itr->second) });
    OTHER_ASSERT(script_success, "Failed to insert loaded Lua script into map.");

    return &script_itr->second;
  }

  void lua_host::shutdown() {
    lua_state.collect_garbage();
    loaded_scripts.clear();

    lua_state = sol::state();
  }

}  // namespace other