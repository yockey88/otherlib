/**
 * \file platform/windows/windows_plugin.cpp
 **/
#include "core/defines.hpp"

#include "plugin/plugin.hpp"

#include "windows_library_handle.hpp"

namespace other {

  library_handle* plugin::create_library_handle(const std::string_view plugin_path) {
    return new windows_library_handle(plugin_path);
  }

}  // namespace other