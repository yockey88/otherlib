/**
 * \file file/virtual_file.hpp
 **/
#ifndef OTHER_CORE_FILE_VIRTUAL_FILE_HPP
#define OTHER_CORE_FILE_VIRTUAL_FILE_HPP

#include "file/file_handle.hpp"

namespace other {

  class virtual_file : public file_handle {
   public:
    virtual_file() {
      handle_type = file_type::VIRTUAL;
    }

    virtual_file(const std::string_view name, const std::string_view ext = "")
        : file_handle(name, ext, file_type::VIRTUAL) {}

    /// construct a virtual file pre-loaded with data
    virtual_file(const std::string_view name, const std::string_view ext, std::vector<uint8_t>&& initial_data)
        : file_handle(name, ext, file_type::VIRTUAL), buffer(std::move(initial_data)) {}

    ~virtual_file() override {
      close();
    }

    bool exists() const override { return true; }
    uint64_t size() const override { return static_cast<uint64_t>(buffer.size()); }

    bool open(file_mode mode) override;
    void close() override;

    std::vector<uint8_t> read_all() override;
    uint64_t read(std::span<uint8_t> out, uint64_t offset = 0) override;
    uint64_t write(std::span<const uint8_t> data) override;

    /// direct access to the underlying buffer for zero-copy reads
    const std::vector<uint8_t>& raw_buffer() const { return buffer; }
    std::vector<uint8_t>& raw_buffer() { return buffer; }

    /// resets the read/write cursor to the beginning
    void rewind() { cursor = 0; }

    /// clears the buffer entirely
    void clear() {
      buffer.clear();
      cursor = 0;
    }

    /// reserves capacity in the backing buffer
    void reserve(uint64_t capacity) { buffer.reserve(static_cast<size_t>(capacity)); }

   private:
    std::vector<uint8_t> buffer;
    uint64_t cursor = 0;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_VIRTUAL_FILE_HPP