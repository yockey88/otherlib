/**
 * \file scripting/dotnet_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP

#include "dotnet/host.hpp"

namespace other {

  void bind_otherlib_dotnet_functions(dotnet_host& dn_host);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP