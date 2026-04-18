/**
 * \file filesystem/remote_file.hpp
 **/
#ifndef OTHER_CORE_FILE_REMOTE_FILE_HPP
#define OTHER_CORE_FILE_REMOTE_FILE_HPP

#include "core/coroutine.hpp"
#include "file/file_handle.hpp"

namespace other {

  enum class remote_fetch_state : uint8_t {
    IDLE = 0,  /// not yet fetched
    FETCHING,
    COMPLETE,
    FAILED,
  };

  class remote_file : public file_handle {
   public:
    remote_file() {
      handle_type = file_type::REMOTE;
    }

    remote_file(event_system& events, const std::string_view name, const std::string_view ext, const std::string_view url)
        : file_handle(events, name, ext, filepath(), std::string{ url }, file_type::REMOTE), remote_url(url) {}

    ~remote_file() override {
      close();
    }

    bool exists() const override {
      return fetch_state == remote_fetch_state::COMPLETE && !cached_data.empty();
    }

    uint64_t size() const override {
      return static_cast<uint64_t>(cached_data.size());
    }

    bool open(file_mode mode) override;
    void close() override;

    std::vector<uint8_t> read_all() override;
    uint64_t read(std::span<uint8_t> buffer, uint64_t offset = 0) override;
    uint64_t write(std::span<const uint8_t> data) override;

    const std::string& url() const { return remote_url; }
    remote_fetch_state state() const { return fetch_state; }

    /// \note stub - implement the actual network request in the body of this coroutine
    task fetch();

    /// returns true if the data has been fetched and is ready to read
    bool is_fetched() const { return fetch_state == remote_fetch_state::COMPLETE; }

   private:
    std::string remote_url;
    std::vector<uint8_t> cached_data;
    remote_fetch_state fetch_state = remote_fetch_state::IDLE;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_REMOTE_FILE_HPP