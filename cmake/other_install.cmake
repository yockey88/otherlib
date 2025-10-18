function(other_install_module TARGET_NAME MODULE_NAME)
  set(options "")
  set(one_value_args HEADER_DIR)
  set(multi_value_args HEADER_PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  
  if(NOT ARG_HEADER_DIR)
    set(ARG_HEADER_DIR "src")
  endif()
  
  if(NOT ARG_HEADER_PATTERNS)
    set(ARG_HEADER_PATTERNS "*.hpp")
  endif()
  
  ## Install the target
  install(
    TARGETS ${TARGET_NAME}
    EXPORT other-targets
    LIBRARY DESTINATION "${OTHER_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OTHER_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${OTHER_INSTALL_INCLUDEDIR}"
  )
  
  ## Install headers - root level or module subdirectory
  if("${MODULE_NAME}" STREQUAL "")
    set(HEADER_DEST "${OTHER_INSTALL_INCLUDEDIR}")
  else()
    set(HEADER_DEST "${OTHER_INSTALL_INCLUDEDIR}/${MODULE_NAME}")
  endif()
  
  foreach(PATTERN ${ARG_HEADER_PATTERNS})
    install(
      DIRECTORY "${ARG_HEADER_DIR}/"
      DESTINATION "${HEADER_DEST}"
      FILES_MATCHING PATTERN "${PATTERN}"
    )
  endforeach()
endfunction()

function(other_install_dependency TARGET_NAME HEADER_DIR)
  set(options "")
  set(one_value_args "")
  set(multi_value_args HEADER_PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  
  if(NOT ARG_HEADER_PATTERNS)
    set(ARG_HEADER_PATTERNS "*.h" "*.hpp")
  endif()
  
  ## Install the target
  install(
    TARGETS ${TARGET_NAME}
    EXPORT other-targets
    LIBRARY DESTINATION "${OTHER_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OTHER_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${OTHER_INSTALL_INCLUDEDIR}"
  )
  
  ## Install headers
  foreach(PATTERN ${ARG_HEADER_PATTERNS})
    install(
      DIRECTORY "${HEADER_DIR}/"
      DESTINATION "${OTHER_INSTALL_INCLUDEDIR}/third_party/${TARGET_NAME}"
      FILES_MATCHING PATTERN "${PATTERN}"
    )
  endforeach()
endfunction()

function(other_install_external_headers LIB_NAME HEADER_DIR)
  set(options "")
  set(one_value_args "")
  set(multi_value_args HEADER_PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  
  if(NOT ARG_HEADER_PATTERNS)
    set(ARG_HEADER_PATTERNS "*.h" "*.hpp" "*.inl")
  endif()
  
  foreach(PATTERN ${ARG_HEADER_PATTERNS})
    install(
      DIRECTORY "${HEADER_DIR}/"
      DESTINATION "${OTHER_INSTALL_INCLUDEDIR}/extern/${LIB_NAME}"
      FILES_MATCHING PATTERN "${PATTERN}"
    )
  endforeach()
endfunction()

function(other_install_csharp TARGET_NAME)
  install(
    TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION "${OTHER_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OTHER_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}"
  )
endfunction()