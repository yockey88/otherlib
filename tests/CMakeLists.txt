if (NOT TARGET gtest)
  add_subdirectory(gtest)
endif()

add_executable(other_tests)
target_compile_definitions(other_tests PRIVATE "OTHER_TEST_ENVIRONMENT")
target_sources(other_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/arena_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ref/ref_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/value/value_test.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/other_test_main.cpp
)
target_link_libraries(other_tests PRIVATE otherlib gtest)