if (NOT TARGET gtest)
  add_subdirectory(gtest)
endif()

enable_testing()

add_executable(other_tests)
target_compile_definitions(other_tests PRIVATE OTHER_TEST_ENVIRONMENT)
target_sources(other_tests
    PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/fuzzing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/config_table_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/reflection_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/state_machine_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/job_system_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/filesystem_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_system_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/logger_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/cli/cli_tool_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli/scene_tool_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/arena/arena_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/arena_buffer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/scope_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/memory_pool_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/graph_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/spsc_buffer_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/ref/ref_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ref/weak_ref_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/value/value_test.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/serialization_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/scene_document_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-combinator/parser_combinator_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/thread_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/message/message_bus_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/script_environment_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/callback_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/action_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/lua_script_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_serializer_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/asset_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/physics/simulation_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/shape_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/contact_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/debug_and_lifecycle_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/box3d_backend_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_environment_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_import_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_scene_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/model_importer_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/animation_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/material_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/assets/glob_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/asset_resolver_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/file_watcher_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/network/network_thread_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/network/transport_provider_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/network_thread_test_runner.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/vm/lexer_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/simple_parsing_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/simple_compiling_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/simple_linking_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/oasm_parsing_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/oasm_parsing2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/ocmd_compiler_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/ocmd_linker_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm/vm_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/scratch_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/other_test_main.cpp
)
target_include_directories(other_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(other_tests PUBLIC otherlib gmock)

# TODO: fix this, it tries to discover tests during build, but DLLs don't get copied over until after build
#        so it fails to find them and errors out, breaking the pipeline
# include(GoogleTest)
# gtest_discover_tests(other_tests)

add_subdirectory(harness)