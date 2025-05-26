add_executable(other_driver)
target_sources(other_driver PRIVATE other_driver.cpp)
target_link_libraries(other_driver PRIVATE otherlib)

function (copy_dlls target)
  if (WIN32)
    add_custom_command (
        TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${target}>"
                "$<TARGET_RUNTIME_DLLS:${target}>" USES_TERMINAL COMMAND_EXPAND_LISTS
    )
  endif ()
endfunction ()

### copy sdl3.dll to output directory
copy_dlls(other_driver)