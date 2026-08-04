add_library(other_audio STATIC)
target_sources(other_audio
  PRIVATE
    src/audio/audio_environment.cpp
    src/audio/miniaudio_impl.cpp
)

target_include_directories(other_audio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/audio>
)
target_link_directories(other_audio
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

## decoders trimmed to the extension table (.wav/.mp3); flags are PRIVATE because no
##  header outside this module includes miniaudio (audio_environment is pimpl'd)
target_compile_definitions(other_audio
  PRIVATE
    MA_NO_ENCODING
    MA_NO_GENERATION
    MA_NO_FLAC
)

target_link_libraries(other_audio
  PRIVATE
    other_core
)

other_install_module(other_audio "audio")
