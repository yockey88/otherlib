/**
 * \file file/local_file.hpp
 **/
#ifndef OTHER_CORE_FILE_LOCAL_FILE_HPP
#define OTHER_CORE_FILE_LOCAL_FILE_HPP

#include "file/file_handle.hpp"

namespace other {

  class local_file : public file_handle {
   public:
    local_file() = default;

    local_file(const filepath& path)
        : file_handle(path.filename().stem().string(), path.extension().string(), file_type::LOCAL) {
      abs_path = std::filesystem::absolute(path);
      file_name = path.filename().string();
    }

    ~local_file() override {
      close();
    }

    bool exists() const override;
    natural_t size() const override;
    bool open(file_mode mode) override;
    void close() override;

    std::vector<uint8_t> read_all() override;
    natural_t read(std::span<uint8_t> buffer, natural_t offset = 0) override;
    natural_t write(std::span<const uint8_t> data) override;

   private:
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_LOCAL_FILE_HPP