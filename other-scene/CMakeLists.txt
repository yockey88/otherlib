add_library(other_scene STATIC)
target_sources(other_scene
    PRIVATE
      src/asset/asset_serialization.cpp
      src/asset/asset.cpp
      src/asset/asset_pipeline.cpp
      src/asset/model_source_pipeline.cpp
      src/asset/asset_handler.cpp

      src/object/component.cpp
      src/object/scene_object.cpp
      src/object/script_component.cpp
      src/object/physics_component.cpp
      src/object/animation_controller.cpp
      src/object/object_serialization.cpp
      
      src/scene/scene_network_context.cpp
      src/scene/scene_tree.cpp
      src/scene/scene_storage.cpp
      src/scene/scene.cpp
      src/scene/scene_graph.cpp
      src/scene/scene_serialization.cpp
)

target_include_directories(other_scene 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/scene>
)

target_link_directories(other_scene PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_scene PRIVATE other_audio other_renderer other_physics other_scripting other_network other_core)

other_install_module(other_scene "scene")