/**
 * \file scripting/bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_BINDINGS_HPP

#include "dotnet/dotnet_host.hpp"
#include "lua/lua_host.hpp"
#include "lua/sol_bridge.hpp"

namespace other {

  class driver;
  class dotnet_object;

  void bind_otherlib_dotnet_functions(dotnet_host& dn_host);
  void bind_otherlib_lua_functions(lua_host& lua_host);

  void do_script_interface_bindings(driver* drv);
  void do_script_interface_unbinding();

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDINGS_HPP
