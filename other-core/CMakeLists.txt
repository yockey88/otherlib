add_library(other_core STATIC)
target_compile_definitions(other_core PUBLIC SPDLOG_USE_STD_FORMAT=1 "ASIO_STANDALONE" "ASIO_HAS_STD_INVOKE_RESULT")

target_sources(other_core
  PRIVATE
		src/core/subsystem.cpp
		src/core/command_line.cpp
		src/core/config_table.cpp
		src/core/defines.cpp
		src/core/logger.cpp
		src/core/page.cpp
		src/core/arena_storage.cpp
		src/core/arena.cpp
		src/core/frame_allocator.cpp
		src/core/arena_buffer.cpp
		src/core/ref_counted.cpp
		src/core/value_storage.cpp
		src/core/value.cpp
		src/core/formatting.cpp
		src/core/job.cpp
		src/core/job_graph.cpp
		src/core/job_system.cpp
		src/core/coroutine.cpp

		src/file/file_watcher.cpp
		src/file/file_handle.cpp
		src/file/local_file.cpp
		src/file/virtual_file.cpp
		src/file/remote_file.cpp
		src/file/directory.cpp
		src/file/filesystem.cpp

		src/data-structures/arena_vector.cpp
		src/data-structures/graph.cpp

		src/math/morton_codes.cpp
		src/math/random.cpp
		src/math/orthonormal_basis.cpp
		src/math/coordinate_frame.cpp
		src/math/interval.cpp
		src/math/bounding_box.cpp
		src/math/plane.cpp
		src/math/ray.cpp
		src/math/frustum.cpp
		src/math/integrator.cpp
		src/math/vector.cpp
		src/math/matrix.cpp
		src/math/dynamic_vector.cpp

		src/serialization/reflection.cpp
		src/serialization/parser_combinators.cpp
		src/serialization/serialization.cpp

		src/message/message_bus.cpp
		src/message/messages.cpp

		src/thread/thread_safety.cpp
		src/thread/thread.cpp

		src/event/event_system.cpp

		src/input/input_action.cpp
		src/input/input_state.cpp
		src/input/input_system.cpp
)

target_include_directories(other_core 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/core>		
)

target_precompile_headers(other_core PUBLIC
	<memory> 
	<optional> 
	<cstdint> 
	<filesystem> 
  <algorithm> 
	<format> 
	<span> 
	<string> 
	<string_view> 
	<type_traits> 
	<vector>
	<functional>
  src/core/build_config.hpp
  src/core/defines.hpp
  src/core/logger.hpp
)

foreach(extern_dir ${OTHER_EXTERN_INCLUDE_DIRS})
  target_include_directories(other_core PUBLIC $<BUILD_INTERFACE:${extern_dir}>)
endforeach()

if (CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
	target_sources(other_core PRIVATE ${OTHER_EXTERN_DIR}/tracy/tracy/TracyClient.cpp)
	target_include_directories(other_core PRIVATE ${OTHER_EXTERN_DIR}/tracy)
endif()

target_link_directories(other_core PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_core PRIVATE glad Threads::Threads)

other_install_module(other_core "core")