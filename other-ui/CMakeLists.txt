add_library(other_ui STATIC)
target_sources(other_ui
  PRIVATE
    src/ui/unicode.cpp
    src/ui/ui_helpers.cpp
    src/ui/ui_node.cpp
    src/ui/ui_window.cpp
)
target_link_libraries(other_ui PRIVATE imgui other_core other_renderer)
target_include_directories(other_ui 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/ui>
)
other_install_module(other_ui "ui")