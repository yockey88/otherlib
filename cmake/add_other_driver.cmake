macro(add_driver_target type driver_name)
  if (${type} STREQUAL "static")
    add_executable(${driver_name})
    target_compile_definitions(${driver_name} PRIVATE "OTHER_APPLICATION")
  elseif (${type} STREQUAL "dynamic")
    add_library(${driver_name} SHARED)
    target_compile_definitions(${driver_name} PRIVATE "OTHER_CLIENT")
  else()
    message(FATAL_ERROR "Unknown driver type: ${type}. Use 'static' or 'dynamic'.")
  endif()
  
  if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL Debug-AS OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
      target_compile_definitions(${driver_name} PRIVATE "/MDd")
    else()
      target_compile_definitions(${driver_name} PRIVATE "/MD")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /utf-8")
  endif()

  if (${BUILD_PLATFORM} STREQUAL "WINDOWS")
    target_compile_definitions(${driver_name} PRIVATE "OTHER_ENVIRONMENT_WINDOWS" ${BUILD_CONFIG_MACRO} "NOMINMAX" "WIN32_LEAN_AND_MEAN" "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
  endif()

  if (${BUILD_PLATFORM} STREQUAL "LINUX")
    target_compile_definitions(${driver_name} PRIVATE "OTHER_ENVIRONMENT_UNIX" ${BUILD_CONFIG_MACRO})
  endif()

  if (CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL Debug-AS OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
    target_compile_definitions(${driver_name} PRIVATE "OTHER_ENVIRONMENT_DEBUG")
  else()
    target_compile_definitions(${driver_name} PRIVATE "OTHER_ENVIRONMENT_RELEASE")
  endif()

  set(driver_src_list "")
  foreach(src_file ${ARGN})
    set(src_file_full_path "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    list(APPEND driver_src_list "${src_file_full_path}")
  endforeach()

  message(STATUS "Adding [${type}] driver: ${driver_name} w/ include directory: ${CMAKE_CURRENT_SOURCE_DIR}")

  target_include_directories(
      ${driver_name} 
    PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}
      ### TODO: replace these with install locations later
      ### all otherlib directories because public linking does not propagate to projects outside solution
      "C:/Yock/code/Other2/OtherEnv/other-core/src"
      "C:/Yock/code/Other2/OtherEnv/other-network/src"
      "C:/Yock/code/Other2/OtherEnv/other-renderer/src"
      "C:/Yock/code/Other2/OtherEnv/other-scene/src"
      "C:/Yock/code/Other2/OtherEnv/other-scripting/src"
      "C:/Yock/code/Other2/OtherEnv/otherlib/src"
      ### also include otherlib externals
      C:/Yock/code/Other2/OtherEnv/extern/magic_enum
      C:/Yock/code/Other2/OtherEnv/extern/glm
      C:/Yock/code/Other2/OtherEnv/extern/dotnet
      C:/Yock/code/Other2/OtherEnv/extern/spdlog
      C:/Yock/code/Other2/OtherEnv/extern/flatbuffers
      C:/Yock/code/Other2/OtherEnv/extern/refl
      C:/Yock/code/Other2/OtherEnv/extern/entt
      C:/Yock/code/Other2/OtherEnv/extern/glad
      C:/Yock/code/Other2/OtherEnv/extern/sdl
      C:/Yock/code/Other2/OtherEnv/extern/imgui
      C:/Yock/code/Other2/OtherEnv/extern/asio
      C:/Yock/code/Other2/OtherEnv/extern/tomlplusplus
  )
  target_sources(${driver_name} PUBLIC ${driver_src_list})

  target_link_directories(
      ${driver_name} 
    PUBLIC 
      "C:/Yock/code/Other2/OtherEnv/build/otherlib"
      "C:/Yock/code/Other2/OtherEnv/build/other-core"
      "C:/Yock/code/Other2/OtherEnv/build/other-network"
      "C:/Yock/code/Other2/OtherEnv/build/other-scene"
      "C:/Yock/code/Other2/OtherEnv/build/other-scripting"
      "C:/Yock/code/Other2/OtherEnv/build/other-renderer"
      ## externals installed in otherlib
      "C:/Yock/code/Other2/OtherEnv/build/extern/imgui"
      ## external libraries that have to be linked
      "C:/Yock/code/Other2/OtherEnv/extern/python312/libs"
      "C:/Yock/code/Other2/OtherEnv/extern/sdl/lib"
  )
  target_link_libraries(${driver_name} PUBLIC otherlib other_core other_network other_scene other_scripting other_renderer SDL3 imgui)
endmacro()

macro(add_static_driver driver_name)
  add_driver_target("static" ${driver_name} ${ARGN})
endmacro()

macro(add_dynamic_driver driver_name)
  add_driver_target("dynamic" ${driver_name} ${ARGN})
endmacro()