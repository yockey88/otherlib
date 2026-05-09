/**
 * \file file/local_file.hpp
 **/
#ifndef OTHER_CORE_FILE_LOCAL_FILE_HPP
#define OTHER_CORE_FILE_LOCAL_FILE_HPP

#include <fstream>

#include "file/file_handle.hpp"

namespace other {

  /// file handle backed by a real file on disk
  class local_file : public file_handle {
   public:
    local_file() = default;

    local_file(event_system& events, const filepath& path, const std::string_view virtual_path)
        : file_handle(events, path.filename().stem().string(), path.extension().string(), std::filesystem::absolute(path), virtual_path, file_type::LOCAL) {}

    ~local_file() override {
      close();
    }

    bool exists() const override {
      return std::filesystem::exists(abs_path);
    }

    uint64_t size() const override {
      if (!std::filesystem::exists(abs_path)) {
        return 0;
      }
      return static_cast<uint64_t>(std::filesystem::file_size(abs_path));
    }

    bool open(file_mode mode) override;
    void close() override;

    std::string read_all_as_string() override;
    std::vector<uint8_t> read_all() override;
    uint64_t read(std::span<uint8_t> buffer, uint64_t offset = 0) override;
    uint64_t write(std::span<const uint8_t> data) override;

   private:
    std::fstream stream;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_LOCAL_FILE_HPP