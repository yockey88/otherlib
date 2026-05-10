/**
 * \file server.cpp
 **/
#include "server.hpp"

#include <filesystem>
#include <string>

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "driver/driver.hpp"

OTHER_DRIVER(other::server)

namespace other {
  namespace {

    std::string lua_name(const std::string& name) {
      return "__server_native_" + name;
    }

  }  // namespace

  void server::on_early_initialize(const command_line& cmd) {
    // http server
    {
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
    add_native_lua_function(lua_name("ReadFileToHttpBody"), [this](const std::string& path) -> std::vector<uint8_t> {
      CORE_LOG_DEBUG("Attempting to read file '{}' to HTTP body", path);
      const filepath full_path = path;
      if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
        std::ifstream file_stream(full_path, std::ios::binary);
        if (!file_stream) {
          CORE_LOG_ERROR("Failed to open file at path: '{}'", full_path.string());
          return {};
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        CORE_LOG_DEBUG(" - read {} bytes from '{}'", data.size(), path);
        return data;
      }
      CORE_LOG_DEBUG(" - attempting to register local file for path '{}'", path);

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "File system subsystem should be available");
      ref<directory> mount = fs->get_mount("server_mount");
      OTHER_ASSERT(mount != nullptr, "Server mount directory not found in file system");

      ref<file_handle> file = mount->get_file(path);
      if (file == nullptr) {
        CORE_LOG_ERROR("File '{}' not found in server mount directory", path);
        return {};
      }

      CORE_LOG_DEBUG(" - read {} bytes from '{}'", file->size(), path);
      return file->read_all();
    });
    add_native_lua_function(lua_name("SendHttpResponse"), [this](natural_t id, const http::response& response) {
      core_system<network_system>().tx_data(id, response.serialize(http::kHttpVersion1_1));
    });
    add_native_lua_function(lua_name("FileExists"), [this](const std::string& path) -> bool {
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

  void server::on_initialize(const command_line& cmd) {
    // http port
    config_http_port = configuration().get_value("server.main-http-port", uint16_t(8080));
    binding_point endpoint{ network_system::network_context::kLocalhostAddress, config_http_port };

    // initialize lua side
    invoke_driver_method("InitializeHttpServer", config_http_port);
    // core_system<network_system>().listen_at_endpoint(endpoint);
  }

  void server::on_shutdown() {
  }

  void server::on_http_request_received(natural_t id, const http::request& req) {
  }

}  // namespace other