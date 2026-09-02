/**
 * \file server.cpp
 **/
#include "server.hpp"

#include <filesystem>
#include <string>

#include "core/defines.hpp"
#include "core/profiler.hpp"

#include "driver/driver.hpp"
#include "driver/systems/network_system.hpp"

#include "message/message.hpp"

OTHER_DRIVER(other::server)

namespace other {
  namespace {

    std::string lua_name(const std::string& name) {
      return "__server_native_" + name;
    }

  }  // namespace

  void server::on_early_initialize() {
    PROFILE_SECTION("server::on_early_initialize");
    // served-content mount
    {
      PROFILE_SECTION("server::on_early_initialize--resolve_mount_directory");
      opt<filepath> directory = std::nullopt;
      if (configuration().has_path("server.mount-directory")) {
        directory = configuration().get_value<std::string>("server.mount-directory");
        if (directory.has_value()) {
          const filepath& dir = directory.value();
          const filepath abspath = std::filesystem::absolute(dir);

          const bool abs_exists = std::filesystem::exists(abspath);
          if (!abs_exists) {
            directory = file_system::get_cwd();
          } else {
            directory = abspath;
          }

          OTHER_ASSERT(!directory.value().empty(), "Mount directory should have a value at this point");
          OTHER_ASSERT(std::filesystem::exists(directory.value()), "Mount directory should exist at this point");
        } else {
          CORE_LOG_ERROR("Failed to parse mount directory from config value: '{}'", configuration().get_value<std::string>("server.mount-directory"));
          directory = file_system::get_cwd();
        }
      }
      OTHER_ASSERT(directory.has_value(), "Mount directory config value is invalid");
      mount_directory = directory.value();
      CORE_LOG_INFO("Server mount directory: '{}'", mount_directory.string());
    }

    {
      PROFILE_SECTION("server::on_early_initialize--mount_server_directory");
      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem should be available");

      ref<directory> mount_dir = fs->mount_directory("server_mount", mount_directory);
      OTHER_ASSERT(mount_dir != nullptr, "Failed to mount server directory");
      CORE_LOG_INFO("Mounted server directory: {}", mount_dir->to_string());
    }

    // configure lua interface
    add_native_lua_function(lua_name("GetMountDirectory"), [this]() {
      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem should be available");

      ref<directory> mount = fs->get_mount("server_mount");
      OTHER_ASSERT(mount != nullptr, "Server mount directory not found in file system");
      return mount->absolute_path().string();
    });
    add_native_lua_function(lua_name("FileExists"), [this](const std::string& path) -> bool {
      PROFILE_SECTION("server::lua_file_exists");
      const filepath full_path = path;
      if (std::filesystem::exists(full_path) &&
          std::filesystem::is_regular_file(full_path)) {
        return true;
      }

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem should be available");
      ref<directory> mount = fs->get_mount("server_mount");
      OTHER_ASSERT(mount != nullptr, "Server mount directory not found in file system");

      ref<file_handle> file = mount->get_file(path);
      return file != nullptr;
    });
  }

  void server::on_initialize() {
    PROFILE_SECTION("server::on_initialize");
    config_port = configuration().get_value("server.main-port", uint16_t(8080));
    binding_point endpoint{ network_system::network_context::kLocalhostAddress, config_port };

    invoke_driver_method("InitializeServer", config_port);
    core_system<network_system>().listen({ .binding = endpoint }, "tcp");
  }

  void server::on_shutdown() {
  }

}  // namespace other