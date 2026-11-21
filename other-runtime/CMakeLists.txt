add_static_driver(other_runtime
  src/runtime_driver.cpp
)
target_include_directories(other_runtime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

## a shipped game has no console window. main stays the entry point, so the driver does not
##  change; the two debug-family configurations keep their console
if (MSVC)
  target_link_options(other_runtime PRIVATE
    $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/SUBSYSTEM:WINDOWS>
    $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:/ENTRY:mainCRTStartup>
  )
endif()

install(TARGETS other_runtime RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}/${OTHER_LIB_FAMILY}")