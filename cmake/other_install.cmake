function(other_install_module TARGET_NAME MODULE_NAME)
  set(options "")
  set(one_value_args HEADER_DIR)
  set(multi_value_args HEADER_PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  message(STATUS "[MODULE] ${TARGET_NAME} (${MODULE_NAME}) from ${ARG_HEADER_DIR} to ${OTHER_INSTALL_INCLUDEDIR}/${MODULE_NAME}")
  
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
  
  message(STATUS "[DEPENDENCY] ${TARGET_NAME} from ${HEADER_DIR} to ${OTHER_INSTALL_INCLUDEDIR}/third_party/${TARGET_NAME}")

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
  message(STATUS "[EXTERNAL HEADER] ${LIB_NAME} from ${HEADER_DIR} to ${OTHER_INSTALL_INCLUDEDIR}/extern/${LIB_NAME}")
  
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
  message(STATUS "[CSharp] ${TARGET_NAME} to ${OTHER_INSTALL_BINDIR}")
  install(
    TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION "${OTHER_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OTHER_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}"
  )
endfunction()

function(other_install_external_library LIB_NAME LIBPATH)
  set(options "")
  set(one_value_args "")
  set(multi_value_args FILE_PATTERNS)

  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  message(STATUS "[EXTERNAL LIBRARY] ${LIB_NAME} from ${LIBPATH} to ${OTHER_INSTALL_LIBDIR}")
  
  if(EXISTS "${LIBPATH}" AND EXISTS "${LIBPATH}")
    ## Configuration-specific installation
    install(
      DIRECTORY "${LIBPATH}/"
      DESTINATION "${OTHER_INSTALL_LIBDIR}"
      FILES_MATCHING 
      PATTERN "*.lib"
      PATTERN "*.exp"
      PATTERN "*.pdb"
    )
  else()
   message(FATAL_ERROR "External library path '${LIBPATH}' does not exist for '${LIB_NAME}'")
  endif()
endfunction()