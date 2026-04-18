/**
 * \file file/file_handle.hpp
 **/
#ifndef OTHER_CORE_FILE_FILE_HANDLE_HPP
#define OTHER_CORE_FILE_FILE_HANDLE_HPP

#include "core/ref_counted.hpp"
#include "file/file_watcher.hpp"

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

    inline std::string to_string() const {
      return std::format(
        "File Handle:\n - Type: {}\n - Name: {}\n - Absolute Path: {}\n - Virtual Path: {}",
        handle_type,
        file_name,
        abs_path.string(),
        virt_path
      );
    }

    inline const std::string& name() const { return file_name; }
    inline const std::string& extension() const { return file_extension; }

    inline void set_absolute_path(const filepath& path) { abs_path = path; }
    inline const filepath& absolute_path() const { return abs_path; }

    inline void set_virtual_path(const std::string_view virtual_path) { virt_path = virtual_path; }
    inline const std::string& virtual_path() const { return virt_path; }

    inline file_type type() const { return handle_type; }
    inline file_mode mode() const { return current_mode; }

    void poll();

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

    template <typename OS>
    void print(OS& os, size_t indent_level) const {
      os << std::string(indent_level, ' ') << std::format(" - FILE[{} : {}] : {} ({} bytes)", handle_type, file_name, abs_path.string(), size());
    }

    /// may be null for root-level files
    directory* parent = nullptr;

   protected:
    file_handle() = default;
    file_handle(event_system& events, const std::string_view name, const std::string_view ext, const filepath& abs_path, const std::string_view virtual_path, file_type type);

    std::string file_name;
    std::string file_extension;
    filepath abs_path;
    std::string virt_path;

    file_type handle_type = file_type::INVALID;
    file_mode current_mode = file_mode::CLOSED;

    scope<file_watcher> watcher;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_FILE_HANDLE_HPP