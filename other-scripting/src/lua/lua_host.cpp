/**
 * \file lua/lua_host.cpp
 **/
#include "lua/lua_host.hpp"

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "lua/lua_script.hpp"

#include "sol/load_result.hpp"

namespace other {

  std::string lua_host::sol_object_to_string(const sol::object& obj) {
    if (obj.is<std::string>()) {
      return obj.as<std::string>();
    } else if (obj.is<int>()) {
      return std::to_string(obj.as<int>());
    } else if (obj.is<double>()) {
      return std::to_string(obj.as<double>());
    } else if (obj.is<bool>()) {
      return obj.as<bool>() ? "true" : "false";
    } else {
      return "<unsupported sol-object type>";
    }
  }

  value lua_host::value_from_lua_object(const sol::object& obj) {
    if (obj.is<bool>()) {
      return value(obj.as<bool>());
    } else if (obj.is<int8_t>()) {
      return value(obj.as<int8_t>());
    } else if (obj.is<int16_t>()) {
      return value(obj.as<int16_t>());
    } else if (obj.is<int32_t>()) {
      return value(obj.as<int32_t>());
    } else if (obj.is<int64_t>()) {
      return value(obj.as<int64_t>());
    } else if (obj.is<uint8_t>()) {
      return value(obj.as<uint8_t>());
    } else if (obj.is<uint16_t>()) {
      return value(obj.as<uint16_t>());
    } else if (obj.is<uint32_t>()) {
      return value(obj.as<uint32_t>());
    } else if (obj.is<uint64_t>()) {
      return value(obj.as<uint64_t>());
    } else if (obj.is<float>()) {
      return value(obj.as<float>());
    } else if (obj.is<double>()) {
      return value(obj.as<double>());
    } else if (obj.is<std::string>()) {
      return value(obj.as<std::string>());
    } else {
      CORE_LOG_ERROR("Unsupported Lua object type for conversion to 'other::value'");
      return value{};
    }
  }

  filepath lua_host::get_environment_script_directory() const {
    return script_directory;
  }

  filepath lua_host::retrieve_script_path(const std::string_view script_name) const {
    return script_directory / script_name;
  }

  void lua_host::load_host(const config_table& config) {
    lua_state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::io, sol::lib::os, sol::lib::debug);

    filepath script_dir = config.get_value<std::string>("scripting.other-lua-directory", std::format("{}/lua", get_program_files_folder("OtherEnvironment").string()));
    if (!std::filesystem::exists(script_dir)) {
      CORE_LOG_ERROR("Lua script directory '{}' does not exist.", script_dir.string());
      return;
    }
    script_directory = script_dir;
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
    if (!success || !itr->second.valid()) {
      CORE_LOG_ERROR("Failed to load Lua script '{}'", fpath.string());
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

  sol::table lua_host::try_load_table(const std::string_view file_path) {
    filepath fpath{ file_path };
    if (fpath.extension() != ".lua") {
      CORE_LOG_ERROR("Lua host can only load .lua files, got '{}'", fpath.extension().string());
      return sol::table{};
    }

    auto hash = FNV(fpath.string());
    if (auto itr = loaded_scripts.find(hash); itr != loaded_scripts.end()) {
      sol::protected_function_result result = itr->second();
      if (!result.valid()) {
        sol::error err = result;
        CORE_LOG_ERROR("Failed to execute previously loaded Lua script '{}': {}", fpath.string(), err.what());
        return sol::table{};
      }

      sol::object obj = result;
      if (obj.is<sol::table>()) {
        return obj.as<sol::table>();
      } else {
        CORE_LOG_ERROR("Lua script '{}' did not return a table.", fpath.string());
        return sol::table{};
      }
    }

    auto [itr, success] = loaded_scripts.insert({ hash, lua_state.load_file(fpath.string()) });
    if (!success) {
      CORE_LOG_ERROR("Failed to insert loaded Lua script into map for '{}'", fpath.string());
      return sol::table{};
    }

    sol::protected_function_result result = itr->second();
    if (!result.valid()) {
      sol::error err = result;
      CORE_LOG_ERROR("Failed to execute Lua script '{}': {}", fpath.string(), err.what());
      return sol::table{};
    }

    sol::object obj = result;
    if (obj.is<sol::table>()) {
      return obj.as<sol::table>();
    } else {
      CORE_LOG_ERROR("Lua script '{}' did not return a table.", fpath.string());
      return sol::table{};
    }
  }

  void lua_host::shutdown() {
    lua_state.collect_garbage();
    loaded_scripts.clear();

    lua_state = sol::state();
  }

}  // namespace other