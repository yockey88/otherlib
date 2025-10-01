
add_library(otherlib)
target_sources(otherlib 
    PRIVATE 
        src/plugin/library_handle.cpp
        src/plugin/plugin.cpp
          
        src/driver/driver.cpp

        src/rendering-pipelines/empty_pipeline.cpp
        src/rendering-pipelines/default_instancing_pipeline.cpp
        src/rendering-pipelines/scriptable_pipeline.cpp

        src/other.cpp
)

if (MSVC)
  set(OTHER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src" "${OTHER_EXTERN_INCLUDES}" "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows")
  target_sources(otherlib
      PRIVATE
        platform/windows/windows_library_handle.cpp
        platform/windows/windows_plugin.cpp
  )
else()
  set(OTHER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src" "${OTHER_EXTERN_INCLUDES}" "${CMAKE_CURRENT_SOURCE_DIR}/platform/linux")
endif()

target_include_directories(otherlib PUBLIC ${OTHER_INCLUDES})
target_link_libraries(otherlib PUBLIC other_core other_renderer other_scripting other_scene)