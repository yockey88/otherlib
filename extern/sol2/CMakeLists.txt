add_library(sol2 STATIC)
set_target_properties(sol2 PROPERTIES LINKER_LANGUAGE CXX)
target_sources(sol2
PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lbaselib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lcode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lcorolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lctype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ldblib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ldebug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ldo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ldump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lfunc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lgc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/linit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/liolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lmathlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/loadlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/loslib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lparser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ltests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/ltm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lutf8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lua/lzio.c
)

target_include_directories(sol2 
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sol>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lua>
  $<INSTALL_INTERFACE:include/sol2/sol>
  $<INSTALL_INTERFACE:include/sol2/lua>
)



other_install_dependency(sol2 "${CMAKE_CURRENT_SOURCE_DIR}/sol2" HEADER_PATTERNS "*.h" "*.hpp")