add_library(other_ui STATIC)
target_sources(other_ui
  PRIVATE
    src/ui/unicode.cpp
    src/ui/ui_helpers.cpp
    src/ui/ui_node.cpp
    src/ui/ui_window.cpp
    src/ui/node_editor_math.cpp
    src/ui/node_editor.cpp
)
target_include_directories(other_ui 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/ui>
)
target_precompile_headers(other_ui REUSE_FROM other_core)
target_link_libraries(other_ui PRIVATE imgui other_scene other_renderer other_physics other_scripting other_network other_core)
other_install_module(other_ui "ui")