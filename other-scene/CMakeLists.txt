add_library(other_scene STATIC)
target_sources(other_scene
    PRIVATE
      src/tools/tool.cpp
      src/tools/project_tool.cpp

      src/asset/asset_serialization.cpp
      src/asset/asset.cpp
      src/asset/asset_pipeline.cpp
      src/asset/asset_resolver.cpp
      src/asset/asset_handler.cpp

      src/object/scene_object.cpp
      src/object/grid_component.cpp
      src/object/script_component.cpp
      src/object/physics_component.cpp
      src/object/component_registry.cpp
      
      src/scene/scene_network_context.cpp
      src/scene/scene_tree.cpp
      src/scene/scene_storage.cpp
      src/scene/scene.cpp
      src/scene/scene_graph.cpp

      src/serialization/animation_serializer.cpp
      src/serialization/component_codec.cpp
      src/serialization/scene_serializer.cpp
)

target_include_directories(other_scene 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/scene>
)
target_link_directories(other_scene PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_scene PRIVATE other_renderer other_ui other_physics other_audio other_scripting other_network other_core tinyxml2)

other_install_module(other_scene "scene")