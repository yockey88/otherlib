/**
 * \file scripting/dotnet_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP

#include "dotnet/host.hpp"

namespace other {

  class driver;

  void set_dotnet_native_driver(driver* drv);
  void bind_otherlib_dotnet_functions(dotnet_host& dn_host);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP