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
    EXPORT otherlib-targets
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
    EXPORT otherlib-targets
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
    set(ARG_HEADER_PATTERNS "*.h" "*.hpp" "*.inl" "*.hh" "*.ipp")
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

function(other_install_external_library LIB_NAME LIB_BASE_PATH)
  set(options "")
  set(one_value_args "")
  set(multi_value_args FILE_PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  
  if(NOT ARG_FILE_PATTERNS)
    set(ARG_FILE_PATTERNS "*.lib" "*.dll" "*.exp" "*.pdb")
  endif()
  
  if(EXISTS "${LIB_BASE_PATH}/debug" AND EXISTS "${LIB_BASE_PATH}/release")
    ## Configuration-specific installation
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
      install(
        DIRECTORY "${LIB_BASE_PATH}/debug/"
        DESTINATION "${OTHER_INSTALL_LIBDIR}"
        FILES_MATCHING 
        PATTERN "*.lib"
        PATTERN "*.exp"
        PATTERN "*.pdb"
      )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "Profile")
      install(
        DIRECTORY "${LIB_BASE_PATH}/release/"
        DESTINATION "${OTHER_INSTALL_LIBDIR}"
        FILES_MATCHING 
        PATTERN "*.lib"
        PATTERN "*.exp"
        PATTERN "*.pdb"
      )
    endif()
  else()
    ## Single configuration installation
    install(
      DIRECTORY "${LIB_BASE_PATH}/"
      DESTINATION "${OTHER_INSTALL_LIBDIR}"
      FILES_MATCHING 
      PATTERN "*.lib"
    )
  endif()
endfunction()