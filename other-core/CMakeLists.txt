add_library(other_core STATIC)
target_compile_definitions(other_core PUBLIC SPDLOG_USE_STD_FORMAT=1 "ASIO_STANDALONE" "ASIO_HAS_STD_INVOKE_RESULT")

target_sources(other_core
  PRIVATE
      src/core/command_line.cpp
      src/core/config_table.cpp
      src/core/defines.cpp
      src/core/logger.cpp
      src/core/arena.cpp
      src/core/registers.cpp
      src/core/ref.cpp
      src/core/ref_counted.cpp
      src/core/value.cpp
      src/core/formatting.cpp

      src/math/random.cpp
      src/math/orthonormal_basis.cpp
      src/math/interval.cpp
      src/math/ray.cpp
      src/math/integrator.cpp

      src/serialization/reflection.cpp
      src/serialization/parser_combinators.cpp
      src/serialization/command_parser.cpp

      src/thread/message.cpp
)

target_include_directories(other_core 
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/src" 
        ${OTHER_EXTERN_DIR}/magic_enum
        ${OTHER_EXTERN_DIR}/args
        ${OTHER_EXTERN_DIR}/tomlplusplus
        ${OTHER_EXTERN_DIR}/spdlog
        ${OTHER_EXTERN_DIR}/glm
        ${OTHER_EXTERN_DIR}/refl
        ${OTHER_EXTERN_DIR}/reflect-cpp
        ${OTHER_EXTERN_DIR}/flatbuffers
        ${OTHER_EXTERN_DIR}/asio
)
target_link_directories(other_core PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_core PUBLIC flatbuffers Threads::Threads)