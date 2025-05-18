cmake_minimum_required(VERSION 3.10.0..4.0.1 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(otherenv LANGUAGES C CXX)

set(OTHERENV_WINDOWS_BUILD false)
set(OTHERENV_LINUX_BUILD false)
set(OTHERENV_OSX_BUILD false)

set(OTHERENV_DEFINES "")

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(OTHERENV_WINDOWS_BUILD true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(OTHERENV_LINUX_BUILD true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(OTHERENV_OSX_BUILD true)   
endif() 
  
if (OTHERENV_WINDOWS_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10+
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
  set(OTHERENV_DEFINES ${OTHERENV_DEFINES} OTHERENV_WINDOWS)
elseif (OTHERENV_LINUX_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  set(OTHERENV_DEFINES ${OTHERENV_DEFINES} OTHERENV_LINUX)
elseif (OTHERENV_OSX_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  set(OTHERENV_DEFINES ${OTHERENV_DEFINES} OTHERENV_OSX)
else()
  message(FATAL_ERROR "Unknown OS")
endif()

macro (add_system_library ${name} ${ARGN})
  set(LIB_SOURCE_FILES_LIST "${ARGN}")
  add_library(${name})

  set(SRC_FILE_STR "")
  foreach (SRC_FILE IN LISTS LIB_SOURCE_FILES_LIST)
    list(APPEND SRC_FILE_STR "${SRC_FILE}")
  endforeach()

  target_sources(${name} PRIVATE ${SRC_FILE_STR})
  target_compile_definitions(${name} PRIVATE ${OTHERENV_DEFINES})
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PUBLIC tensorlib)

  add_executable(${name}_driver)

  target_include_directories(${name}_driver PRIVATE ${OTHERENV_INCLUDES})
  target_sources(${name}_driver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp)
  target_link_libraries(${name}_driver PRIVATE tensorlib network)
endmacro()


find_package(Threads REQUIRED)

## core other environment library
add_subdirectory(otherenv)

## collection of subsystem libraries

## main driver
add_executable(otherenv_driver)
target_include_directories(otherenv_driver PRIVATE otherenv/src)
target_sources(otherenv_driver
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/otherenv_driver.cpp
)
target_link_libraries(otherenv_driver PRIVATE otherenv Threads::Threads)