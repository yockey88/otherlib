function(configure_version_header FILE_NAME HEADER_NAME NAME MAJOR MINOR PATCH OUTPUT_PATH)
  set(CMAKE_FILE_NAME "${FILE_NAME}")
  set(CMAKE_HEADER_NAME "${HEADER_NAME}")
  set(CMAKE_NAME "${NAME}")
  set(CMAKE_VERSION_MAJOR "${MAJOR}")
  set(CMAKE_VERSION_MINOR "${MINOR}")
  set(CMAKE_VERSION_PATCH "${PATCH}")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/version.hpp.in
    ${OUTPUT_PATH}
    @ONLY
  )
endfunction()