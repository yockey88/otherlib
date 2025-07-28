add_library(other_scene STATIC)
target_sources(other_scene
    PRIVATE
      src/object/scene_object.cpp
      src/object/script_component.cpp
      
      src/scene/scene_tree.cpp
      src/scene/scene.cpp
)
if (MSVC)
else()
endif()

target_include_directories(other_scene 
  PUBLIC 
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      ${OTHER_EXTERN_DIR}/entt
)
target_link_directories(other_scene PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_scene PUBLIC other_core other_renderer other_scripting)