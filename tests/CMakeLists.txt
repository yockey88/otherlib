if (NOT TARGET gtest)
  add_subdirectory(gtest)
endif()

enable_testing()

add_executable(other_tests)
target_compile_definitions(other_tests PRIVATE "OTHER_TEST_ENVIRONMENT")
target_sources(other_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/reflection_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/state_machine_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/arena/arena_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/arena_buffer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/scope_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/arena_vector_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/arena_array_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/memory_pool_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data-structures/graph_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/ref/ref_test.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/value/value_test.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/script_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/scene_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/config_table_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/asset_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/ocl_object_parser_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/parser-combinator/parser_combinator_tests.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/thread_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/message_bus_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/script_environment_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/action_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/lua_script_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/asset_tests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/other_test_main.cpp
)
target_include_directories(other_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(other_tests PUBLIC otherlib gmock)

# TODO: fix this, it tries to discover tests during build, but DLLs don't get copied over until after build
#        so it fails to find them and errors out, breaking the pipeline
# include(GoogleTest)
# gtest_discover_tests(other_tests)