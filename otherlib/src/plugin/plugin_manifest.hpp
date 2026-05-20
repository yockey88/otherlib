/**
 * \file plugin/plugin_manifest.hpp
 **/
#ifndef OTHERLIB_PLUGIN_PLUGIN_MANIFEST_HPP
#define OTHERLIB_PLUGIN_PLUGIN_MANIFEST_HPP

#include "core/defines.hpp"

namespace other {

  struct plugin_manifest {
    natural_t interface_hash;
    void* (*factory_function)();

    const char* class_name;
    const char* plugin_instance_name;
  };

}  // namespace other

#endif  // OTHERLIB_PLUGIN_PLUGIN_MANIFEST_HPP