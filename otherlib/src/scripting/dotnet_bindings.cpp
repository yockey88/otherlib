/**
 * \file scripting/dotnet_bindings.cpp
 **/
#include "scripting/dotnet_bindings.hpp"

#include <imgui/imgui.h>

#include "script/scripting_environment.hpp"

namespace other {
  namespace bindings {

    /// Filesystem.

    native_string native_get_program_files_folder(native_string app_name_str) {
      std::string app_name = app_name_str;
      filepath program_files_folder = get_program_files_folder(app_name);
      return native_string::new_str(program_files_folder.string());
    }

    native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag) {
      std::string app_name = app_name_str;
      filepath app_data_folder = get_app_data_folder(app_name, create_flag != 0);
      return native_string::new_str(app_data_folder.string());
    }

    native_string native_get_install_folder() {
      filepath install_folder = get_other_environment_install_folder();
      return native_string::new_str(install_folder.string());
    }

    /// UI.

    bool native_begin_window(native_string title, int32_t flags) {
      std::string title_str = title;
      return ImGui::Begin(title_str.c_str(), nullptr, static_cast<ImGuiWindowFlags>(flags));
    }

    void native_end_window() {
      ImGui::End();
    }

    bool native_begin_child(native_string str_id, ImVec2 size, bool border, int32_t flags) {
      std::string id = str_id;
      return ImGui::BeginChild(id.c_str(), size, border, static_cast<ImGuiChildFlags>(flags));
    }

    void native_end_child() {
      ImGui::EndChild();
    }

    ///

    template <typename Fn>
    void bind_function(dotnet_host& dn_host, native_string name, Fn fn) {
      PROFILE_SECTION("other::bindings::bind-function");
      void* fn_ptr = (void*)fn;
      dn_host.interop().bind_native_function(name, fn_ptr);
    }

    void validate_binding_points(dotnet_host& dn_host) {
      PROFILE_SECTION("other::bindings::validate-binding-points");
      nbool32 res = dn_host.interop().validate_binding_points();
      if (!res) {
        CORE_LOG_ERROR("One or more native functions failed to bind to managed counterparts.");
      }
    }

    struct binding_context {
      dotnet_host& host;

      binding_context(dotnet_host& h)
          : host(h) {}

      template <typename Fn>
      binding_context& bind(const std::string_view name, Fn fn) {
        PROFILE_SECTION("other::bindings::binding_context::bind");
        native_scoped_string fn_name = native_string::new_str(name);
        bind_function(host, fn_name, fn);
        return *this;
      }
    };

  }  // namespace bindings

  void bind_otherlib_dotnet_functions(dotnet_host& dn_host) {
    PROFILE_SECTION("other::bind-otherlib-dotnet-functions");
    dn_host.rediscover_binding_points();

    bindings::binding_context{ dn_host }
      /// Filesystem.
      .bind("GetProgramFilesFolder", bindings::native_get_program_files_folder)
      .bind("GetAppDataFolder", bindings::native_get_app_data_folder)
      .bind("GetInstallFolder", bindings::native_get_install_folder);

    bindings::binding_context{ dn_host }
      /// UI.
      .bind("BeginWindow", bindings::native_begin_window)
      .bind("EndWindow", bindings::native_end_window)
      .bind("BeginChild", bindings::native_begin_child)
      .bind("EndChild", bindings::native_end_child);

    bindings::validate_binding_points(dn_host);
  }

}  // namespace other