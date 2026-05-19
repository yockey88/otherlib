add_library(other_audio STATIC)
target_sources(other_audio
    PRIVATE
      src/audio_device/audio_device.cpp
      src/audio/audio_engine.cpp
)

target_include_directories(other_audio 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/audio>
)
target_precompile_headers(other_audio REUSE_FROM other_core)
target_link_directories(other_audio PUBLIC 
  $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
  $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)
target_link_libraries(other_audio PRIVATE miniaudio other_core)

other_install_module(other_audio "audio")