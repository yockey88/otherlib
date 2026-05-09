/**
 * \file plugin/library_handle.hpp
 **/
#ifndef OTHERLIB_PLUGIN_LIBRARY_HANDLE_HPP
#define OTHERLIB_PLUGIN_LIBRARY_HANDLE_HPP

#include <expected>
#include <map>
#include <string_view>

#include "core/defines.hpp"

namespace other {

  struct other_plugin_argv;

  struct symbol {
    void* address;

    template <typename Fn>
    Fn get_function() const {
      return reinterpret_cast<Fn>(address);
    }
  };

  class library_handle {
   public:
    library_handle(const std::string_view filepath)
        : filepath(filepath) {}
    virtual ~library_handle() {}

    virtual void load() = 0;
    virtual bool is_loaded() const = 0;
    opt<symbol> get_symbol(const std::string_view symbol);

    virtual void unload() = 0;

   protected:
    std::string filepath;

    std::map<natural_t, symbol> symbols;

    virtual symbol load_symbol(const std::string_view symbol) = 0;
  };

}  // namespace other

#endif  // OTHERLIB_PLUGIN_LIBRARY_HANDLE_HPP