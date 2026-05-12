add_library(other_management_plugin SHARED 
  management-plugin/other_editor_management.cpp
  management-plugin/tcp_listener.cpp
  management-plugin/status_window.cpp
)
target_link_libraries(other_management_plugin PRIVATE otherlib)
target_compile_definitions(other_management_plugin PRIVATE
  OTHER_PLUGIN_LIBRARY
)