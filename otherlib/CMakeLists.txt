set(OTHERLIB_VERSION_MAJOR "0")
set(OTHERLIB_VERSION_MINOR "0")
set(OTHERLIB_VERSION_PATCH "1")
configure_version_header(
  "otherlib_version.hpp"
  "OTHERLIB"
  "OTHERLIB"
  "${OTHERLIB_VERSION_MAJOR}"
  "${OTHERLIB_VERSION_MINOR}"
  "${OTHERLIB_VERSION_PATCH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/otherlib_version.hpp"
)

set(OCMD_FILE_FORMAT_VERSION_MAJOR "0")
set(OCMD_FILE_FORMAT_VERSION_MINOR "1")
set(OCMD_FILE_FORMAT_VERSION_PATCH "0")
configure_version_header(
  "ocmd_file_format_version.hpp"
  "OTHERLIB_VM_COMMAND_FILES_OCMD_FILE_FORMAT"
  "OCMD_FILE_FORMAT"
  "${OCMD_FILE_FORMAT_VERSION_MAJOR}"
  "${OCMD_FILE_FORMAT_VERSION_MINOR}"
  "${OCMD_FILE_FORMAT_VERSION_PATCH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/vm/ocmd_file_format_version.hpp"
)


add_library(otherlib)
target_sources(otherlib 
  PRIVATE 
    src/plugin/library_handle.cpp
    src/plugin/plugin.cpp
      
    src/driver/driver.cpp

    src/rendering-pipelines/empty_pipeline.cpp
    src/rendering-pipelines/default_instancing_pipeline.cpp
    src/rendering-pipelines/scriptable_pipeline.cpp

    src/scripting/dotnet_bindings.cpp
    src/scripting/lua_bindings.cpp
    src/scripting/scene_interface.cpp
    src/scripting/binding_utils/bind_glm_vectors.cpp
    src/scripting/binding_utils/bind_glm_matrices.cpp
    src/scripting/binding_utils/bind_math_types_lua.cpp

    src/scripting/actions/action.cpp
    src/scripting/actions/callback.cpp

    src/scripting/execution_node.cpp
    src/scripting/execution_graph.cpp
    src/scripting/execution_nodes/source_sink_nodes.cpp
    src/scripting/execution_nodes/arithmetic_nodes.cpp
    src/scripting/execution_nodes/linear_algebra_nodes.cpp
    src/scripting/execution_nodes/transform_nodes.cpp

    src/vm/opcode.cpp
    src/vm/driver_interface.cpp
    src/vm/control_table.cpp
    src/vm/other_device.cpp
    src/vm/decompiler.cpp
    src/vm/vm.cpp
    src/vm/command_files/token.cpp
    src/vm/command_files/code_block.cpp
    src/vm/command_files/data_block.cpp
    src/vm/command_files/lexer.cpp
    src/vm/command_files/parser.cpp
    src/vm/command_files/assembler.cpp
    src/vm/command_files/linker.cpp
    src/vm/command_files/ocmd_compiler.cpp

    src/tools/build_tool.cpp
    src/tools/environment_console.cpp

    src/ui/ui_widgets.cpp
    src/ui/node_editor_canvas_node.cpp
    src/ui/node_editor.cpp
    src/ui/console_history_node.cpp
    src/ui/console.cpp
    src/ui/viewport.cpp
    src/ui/type_database.cpp
    src/ui/driver_ui.cpp
    src/ui/type-bindings/value_ui.cpp
    src/ui/type-bindings/vm_ui.cpp
    src/ui/type-bindings/scene_ui.cpp

    src/other.cpp
)

if (MSVC)
  target_sources(otherlib
      PRIVATE
        platform/windows/windows_library_handle.cpp
        platform/windows/windows_plugin.cpp
  )
  set(PLATFORM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows")
else()
  set(PLATFORM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/linux")
endif()

target_include_directories(otherlib 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PLATFORM_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}>
)

foreach(extern_dir ${OTHER_EXTERN_INCLUDE_DIRS})
  target_include_directories(otherlib PUBLIC $<BUILD_INTERFACE:${extern_dir}>)
endforeach()

target_link_directories(otherlib PUBLIC 
  $<BUILD_INTERFACE:${OTHER_EXTERN_DIR}/sol2/lib>
)
target_link_libraries(otherlib PUBLIC other_scene other_renderer other_scripting other_network other_core)

if (CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL Debug-AS OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
  target_link_libraries(otherlib PRIVATE nfd_d)
else()
  target_link_libraries(otherlib PRIVATE nfd)
endif()

add_library(otherlib_main STATIC src/other_main.cpp)
target_link_libraries(otherlib_main PUBLIC otherlib)
target_include_directories(otherlib_main PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}>
)

other_install_module(otherlib "")
install(
  TARGETS otherlib_main
  EXPORT otherlib-targets
  LIBRARY DESTINATION "${OTHER_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${OTHER_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${OTHER_INSTALL_INCLUDEDIR}"
)