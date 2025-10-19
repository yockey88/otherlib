# function(QUERY_TERMINAL command)
#   execute_process(COMMAND cmd.exe /c ${command} OUTPUT_VARIABLE result)
#   set(QUERY_TERMINAL_RESULT ${result} PARENT_SCOPE)
# endfunction()

# function(GET_PYTHON_PATH)
#   QUERY_TERMINAL(python -c "import sys; import os; print(os.path.dirname(sys.executable))")
#   string(STRIP "${QUERY_TERMINAL_RESULT}" python_path)
#   set(PYTHON_PATH ${python_path} PARENT_SCOPE)
#   set(PYTHON_INCLUDE_DIR ${PYTHON_PATH}/include PARENT_SCOPE)
#   set(PYTHON_LIB_DIR ${PYTHON_PATH}/libs PARENT_SCOPE)
# endfunction()

# function(GET_PYTHON_LIB)
#   QUERY_TERMINAL(python -c "import sys; import os; import glob; path = os.path.dirname(sys.executable); libs = glob.glob(path + '/libs/python*'); print(os.path.splitext(os.path.basename(libs[-1]))[0]);")
#   string(STRIP "${QUERY_TERMINAL_RESULT}" python_lib)
#   set(PYTHON_LIB ${python_lib} PARENT_SCOPE)
# endfunction()

# get_python_path()
# get_python_lib()


add_library(other_scripting STATIC)
target_sources(other_scripting
PRIVATE
  src/bindings/native_logger.cpp

  src/dotnet/dotnet_memory.cpp
  src/dotnet/native_string.cpp
  src/dotnet/type_cache.cpp
  src/dotnet/dotnet_method.cpp
  src/dotnet/dotnet_field.cpp
  src/dotnet/dotnet_attribute.cpp
  src/dotnet/dotnet_type.cpp
  src/dotnet/dotnet_assembly.cpp
  src/dotnet/dotnet_object.cpp
  src/dotnet/host.cpp

  src/python/interpreter.cpp

  src/script/script_object.cpp
  src/script/scripting_environment.cpp
)

set(PYTHON_INCLUDE_DIR ${OTHER_EXTERN_DIR}/python312/include)
set(PYTHON_LIB_DIR ${OTHER_EXTERN_DIR}/python312/libs)

message(STATUS "Python Include Directory: ${PYTHON_INCLUDE_DIR}")
message(STATUS "Python Library Directory: ${PYTHON_LIB_DIR}")
message(STATUS "Python Library: ${PYTHON_LIB}")

# find_package(pybind11 REQUIRED)  # or `add_subdirectory(pybind11)`

target_include_directories(other_scripting 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/scripting>
)


target_link_directories(other_scripting 
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}> 
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
    $<BUILD_INTERFACE:${PYTHON_LIB_DIR}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

target_link_libraries(other_scripting PRIVATE ${PYTHON_LIB} other_core)

other_install_module(other_scripting "scripting")