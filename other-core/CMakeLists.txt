add_library(other_core STATIC)
target_compile_definitions(other_core PUBLIC SPDLOG_USE_STD_FORMAT=1 "ASIO_STANDALONE" "ASIO_HAS_STD_INVOKE_RESULT")

target_sources(other_core
  PRIVATE
		src/core/command_line.cpp
		src/core/config_table.cpp
		src/core/defines.cpp
		src/core/logger.cpp
		src/core/arena.cpp
		src/core/arena_buffer.cpp
		src/core/registers.cpp
		src/core/ref.cpp
		src/core/ref_counted.cpp
		src/core/value.cpp
		src/core/formatting.cpp

		src/data-structures/arena_vector.cpp
		src/data-structures/graph.cpp

		src/math/morton_codes.cpp
		src/math/random.cpp
		src/math/orthonormal_basis.cpp
		src/math/coordinate_frame.cpp
		src/math/interval.cpp
		src/math/bounding_box.cpp
		src/math/ray.cpp
		src/math/integrator.cpp
		src/math/vector.cpp
		src/math/matrix.cpp
		src/math/dynamic_vector.cpp
		src/math/dynamic_matrix.cpp
		
		src/command/command_parser.cpp
		src/command/command_compiler.cpp
		src/command/command_executor.cpp

		src/serialization/reflection.cpp
		src/serialization/parser_combinators.cpp
		src/serialization/serialization.cpp

		src/thread/message.cpp
		src/thread/thread.cpp
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

if (CMAKE_BUILD_TYPE STREQUAL Profile OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
	target_sources(other_core PRIVATE ${OTHER_EXTERN_DIR}/tracy/tracy/TracyClient.cpp)
	target_include_directories(other_core PUBLIC ${OTHER_EXTERN_DIR}/tracy)
endif()

target_link_directories(other_core PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_core PUBLIC flatbuffers glad Threads::Threads)