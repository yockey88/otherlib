
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
    src/scripting/other_device.cpp

    src/tools/build_tool.cpp

    src/ui/node_editor.cpp

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