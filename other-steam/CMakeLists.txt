add_library(other_steam STATIC)
target_sources(other_steam
  PRIVATE
    src/steam/steam_context.cpp
    src/steam/steam_lobby.cpp
    src/steam/steam_link_transport.cpp
)

## include root is the SDK folder so sources write #include <steam/steam_api.h>;
##  the SDK itself stays out of public source releases and cpack (S6.4 license pass)
target_include_directories(other_steam
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${OTHER_EXTERN_DIR}/steam>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/steam>
)
target_link_directories(other_steam
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

target_link_libraries(other_steam
  PUBLIC
    other_core
    other_network
    steam_api64
)

other_install_module(other_steam "steam")
