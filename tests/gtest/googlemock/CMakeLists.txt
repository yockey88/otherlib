add_library(gmock STATIC)
target_sources(gmock
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-all.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-cardinalities.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-internal-utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-matchers.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock-spec-builders.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gmock.cc
)
target_include_directories(gmock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(gmock PUBLIC gtest)