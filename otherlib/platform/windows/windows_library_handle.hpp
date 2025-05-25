/**
 * \file windows/windows_library_handle.hpp
 **/
#ifndef OTHER_WINDOWS_LIBRARY_HANDLE_HPP
#define OTHER_WINDOWS_LIBRARY_HANDLE_HPP

#include <windows.h>

#include "plugin/library_handle.hpp"

namespace other {

  class windows_library_handle : public library_handle {
   public:
    windows_library_handle(const std::string_view filepath)
        : library_handle(filepath), handle(nullptr) {}
    virtual ~windows_library_handle() = default;

    void load() override;
    bool is_loaded() const override;
    void unload() override;

   private:
    HMODULE handle;

    symbol load_symbol(const std::string_view symbol) override;
    void print_error() const;
  };

}  // namespace other

#endif  // OTHER_WINDOWS_LIBRARY_HANDLE_HPP