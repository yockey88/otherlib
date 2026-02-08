/**
 * \file file/file_handle.hpp
 **/
#ifndef OTHER_CORE_FILE_FILE_HANDLE_HPP
#define OTHER_CORE_FILE_FILE_HANDLE_HPP

#include "core/ref_counted.hpp"

namespace other {

  struct directory;

  enum class file_type : uint8_t {
    LOCAL = 0,  /// backed by a real file on disk
    VIRTUAL,    /// backed by an in-memory buffer
    REMOTE,     /// backed by a remote resource (fetched lazily via task)

    NUM_FILE_TYPES,
    INVALID = NUM_FILE_TYPES
  };

  enum class file_mode : uint8_t {
    CLOSED = 0,
    READ,
    WRITE,
    READ_WRITE,
  };

  class file_handle : public ref_counted {
   public:
    virtual ~file_handle() = default;

    inline const std::string& name() const { return file_name; }
    inline const std::string& extension() const { return file_extension; }
    inline const filepath& absolute_path() const { return abs_path; }
    inline const std::string& virtual_path() const { return virt_path; }

    inline file_type type() const { return handle_type; }
    inline file_mode mode() const { return current_mode; }

    /// always true for virtual files that have been created
    virtual bool exists() const = 0;
    virtual natural_t size() const = 0;
    bool is_open() const { return current_mode != file_mode::CLOSED; }

    /// returns true on success
    virtual bool open(file_mode mode) = 0;
    virtual void close() = 0;

    virtual std::vector<uint8_t> read_all() = 0;

    /// reads up to `count` bytes starting at `offset` into the provided buffer returns the number of bytes actually read
    /// \note caller is responsible for ensuring the buffer is large enough, environment will assert otherwise
    virtual natural_t read(std::span<uint8_t> buffer, natural_t offset = 0) = 0;
    virtual natural_t write(std::span<const uint8_t> data) = 0;

    /// may be null for root-level files
    directory* parent = nullptr;

   protected:
    file_handle() = default;

    file_handle(const std::string_view name, const std::string_view ext, file_type type)
        : file_name(name), file_extension(ext), handle_type(type) {}

    std::string file_name;
    std::string file_extension;
    filepath abs_path;
    std::string virt_path;

    file_type handle_type = file_type::INVALID;
    file_mode current_mode = file_mode::CLOSED;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_FILE_HANDLE_HPP