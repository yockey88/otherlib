/**
 * \file plugin/library_handle.hpp
 **/
#ifndef OTHER_PLUGIN_LIBRARY_HANDLE_HPP
#define OTHER_PLUGIN_LIBRARY_HANDLE_HPP

#include <map>
#include <string_view>

namespace other {

  struct other_plugin_argv;

  struct symbol {
    std::string_view name;
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
    symbol& get_symbol(const std::string_view symbol);

    virtual void unload() = 0;

   protected:
    std::string_view filepath;

    std::map<uint64_t, symbol> symbols;

    virtual symbol load_symbol(const std::string_view symbol) = 0;

   private:
    friend class plugin;
    void call_plugin_binder(other_plugin_argv* argv);
  };

}  // namespace other

#endif  // OTHER_PLUGIN_LIBRARY_HANDLE_HPP