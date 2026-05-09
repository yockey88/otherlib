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
      
    src/driver/driver_system.cpp
    src/driver/systems/network_system.cpp
    src/driver/systems/job_driver_system.cpp
    src/driver/systems/event_driver_system.cpp
    src/driver/systems/input_driver_system.cpp
    src/driver/systems/asset_system.cpp
    src/driver/systems/scripting_system.cpp
    src/driver/systems/rendering_system.cpp
    src/driver/systems/vm_system.cpp
    src/driver/systems/scene_system.cpp
    src/driver/systems/project_system.cpp
    src/driver/systems/driver_plugin.cpp
    src/driver/driver_kernel.cpp
    src/driver/subsystem_registry.cpp
    src/driver/driver.cpp

    src/ocl/object_parser.cpp

    src/project/project.cpp

    src/scripting/bindings.cpp
    src/scripting/scene_interface.cpp
    src/scripting/binding_descriptor.cpp
    src/scripting/lua_bindings/bind_glm_vectors.cpp
    src/scripting/lua_bindings/bind_glm_matrices.cpp
    src/scripting/lua_bindings/bind_math_types_lua.cpp
    src/scripting/lua_bindings/bind_rendering_types_lua.cpp
    src/scripting/dotnet_bindings/environment_api_bindings.cpp
    src/scripting/dotnet_bindings/core_bindings.cpp
    src/scripting/dotnet_bindings/ui_bindings.cpp
    src/scripting/dotnet_bindings/driver_bindings.cpp
    src/scripting/dotnet_bindings/scene_bindings.cpp
    src/scripting/dotnet_bindings/scene_object_bindings.cpp
    src/scripting/dotnet_bindings/component_bindings.cpp
    src/scripting/dotnet_bindings/event_bindings.cpp
    src/scripting/dotnet_bindings/input_bindings.cpp
    src/scripting/dotnet_bindings/network_bindings.cpp
    src/scripting/interfaces/networking_interfaces.cpp
    src/scripting/interface_registry.cpp
    src/scripting/other_abi.cpp

    src/scripting/actions/action.cpp
    src/scripting/actions/callback.cpp

    src/scripting/execution_nodes/source_sink_nodes.cpp
    src/scripting/execution_nodes/arithmetic_nodes.cpp
    src/scripting/execution_nodes/linear_algebra_nodes.cpp
    src/scripting/execution_nodes/transform_nodes.cpp
    src/scripting/execution_node.cpp
    src/scripting/execution_graph.cpp
    src/scripting/environment_interface.cpp

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

    src/ui/script/script_property_widget.cpp
    src/ui/type-bindings/value_ui.cpp
    src/ui/type-bindings/vm_ui.cpp
    src/ui/type-bindings/scene_ui.cpp
    src/ui/asset-browser/asset_browser_widgets.cpp
    src/ui/asset-browser/asset_browser_tree_node.cpp
    src/ui/asset-browser/asset_browser_grid_node.cpp
    src/ui/asset-browser/asset_browser.cpp
    src/ui/asset-editor/asset_editor_registry.cpp
    src/ui/asset-editor/asset_editor.cpp
    src/ui/asset-editor/asset_editor_node.cpp
    src/ui/console/console_widgets.cpp
    src/ui/console/console_input_node.cpp
    src/ui/console/console_history_node.cpp
    src/ui/console/console.cpp
    src/ui/node-editor/node_editor_canvas_node.cpp
    src/ui/node-editor/node_editor.cpp
    src/ui/scene-hierarchy/hierarchy_widgets.cpp
    src/ui/scene-hierarchy/scene_hierarchy_node.cpp
    src/ui/scene-hierarchy/scene_hierarchy.cpp
    src/ui/menu-bar/menu_item.cpp
    src/ui/menu-bar/menu.cpp
    src/ui/menu-bar/menu_bar.cpp
    src/ui/property_inspector_node.cpp
    src/ui/inspector_widgets.cpp
    src/ui/ui_widgets.cpp
    src/ui/viewport.cpp
    src/ui/type_database.cpp
    src/ui/driver_ui.cpp

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
  target_include_directories(otherlib 
    PUBLIC 
      $<BUILD_INTERFACE:${extern_dir}>
      $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}>    
  )
endforeach()

target_link_directories(otherlib PUBLIC 
  $<BUILD_INTERFACE:${OTHER_EXTERN_DIR}/sol2/lib>
)
target_link_libraries(otherlib PUBLIC other_scene other_renderer other_physics other_scripting other_network other_core)

# if (CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL Debug-AS OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
#   target_link_libraries(otherlib PRIVATE nfd_d)
# else()
#   target_link_libraries(otherlib PRIVATE nfd)
# endif()

add_library(otherlib_main STATIC src/other_main.cpp)
target_link_libraries(otherlib_main PUBLIC otherlib)
target_include_directories(otherlib_main PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}>
)

add_executable(otherlib_driver)
target_sources(otherlib_driver PRIVATE src/other_main.cpp)
target_compile_definitions(otherlib_driver PRIVATE OTHER_CLIENT)
target_link_libraries(otherlib_driver PUBLIC otherlib)
target_include_directories(otherlib_driver PUBLIC
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