/**
 * \file asset/asset_loader.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_LOADER_HPP
#define OTHER_SCENE_ASSET_ASSET_LOADER_HPP

#include <span>
#include <string_view>

#include <asio/asio.hpp>

namespace other {

  // struct asset_loader {
  //   virtual ~asset_loader() = default;

  //   virtual void on_file_read(const std::span<const uint8_t> data) {}
  //   virtual void on_file_read_failed(const std::string_view error_msg) {}

  //   virtual void on_file_close() {}
  //   virtual void on_file_close_failed(const std::string_view error_msg) {}
  // };

  struct model_source_loader {
    enum {
      STOPPED = 0,

      READ_FILE,
      PROCESS_MESHES,

      BAKE_MESHES,
      WRITING_OTHER_MESH_FILE,

      CREATE_MODEL_SOURCE,
      COMPLETE,

      ERROR_STATE,
      NUM_STATES = ERROR_STATE,
    } state = STOPPED;

    template <typename Self>
    void operator()(Self&& self, asio::error_code ec = {}, std::size_t bytes_transferred = 0) {
      switch (state) {
        case STOPPED:
          state = READ_FILE;
          asio::post(asio::get_associated_executor(self), std::forward<Self>(self));
          break;

        case READ_FILE: {
          // TODO
          state = PROCESS_MESHES;
          asio::post(asio::get_associated_executor(self), std::forward<Self>(self));
        } break;

        case PROCESS_MESHES: {
          // TODO
          state = COMPLETE;
          asio::post(asio::get_associated_executor(self), std::forward<Self>(self));
        } break;

        case COMPLETE: {
          // TODO
          self.complete(ec, bytes_transferred);
        } break;

        case ERROR_STATE: {
          // TODO
        } break;

        default:
          break;
      }
    }
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_LOADER_HPP