/**
 * \file scripting/dotnet_bindings/environment_api_bindings.hpp
 **/
#ifndef OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_ENVIRONMENT_API_BINDINGS_HPP
#define OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_ENVIRONMENT_API_BINDINGS_HPP

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    native_string native_get_program_files_folder(native_string app_name_str);
    native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag);
    native_string native_get_install_folder();

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_ENVIRONMENT_API_BINDINGS_HPP