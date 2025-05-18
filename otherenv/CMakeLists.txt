add_library(otherenv STATIC)

target_include_directories(otherenv 
    PUBLIC  
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory
)
target_sources(otherenv
    PRIVATE
        ## 'kernel' files for bedrock utility classes and foundational infrastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/defines.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/ref_counted.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/ref.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/value.cpp

        ## build kernel to create memory allocation utilities and infrastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/arena.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/registers.cpp

        ## last level, only includes user-facing wrappers around otherenv code
        ${CMAKE_CURRENT_SOURCE_DIR}/src/otherenv.cpp
)