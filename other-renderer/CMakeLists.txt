add_library(other_renderer STATIC)
target_sources(other_renderer
    PRIVATE
      src/gpu_resource/renderer_resource.cpp
      src/gpu_resource/shader.cpp
      src/gpu_resource/texture.cpp
      src/gpu_resource/cube_map.cpp
      src/gpu_resource/gpu_buffer.cpp
      src/gpu_resource/mesh.cpp
      src/gpu_resource/framebuffer.cpp
      
      src/model/vertex.cpp
      src/model/vertex_buffer.cpp
      src/model/model_importer.cpp
      src/model/model.cpp
      
      src/renderer/window_manager.cpp
      
      src/renderer/rendering_api.cpp
      src/renderer/renderer_backend.cpp
      src/renderer/backends/opengl_api.cpp

      src/renderer/camera.cpp
      src/renderer/render_graph.cpp
      src/renderer/render_pipeline.cpp
      src/renderer/renderer.cpp

      src/renderer/draw_command.cpp

      src/renderer/ui/nodes/hbox_node.cpp

      src/renderer/ui/ui_helpers.cpp
      src/renderer/ui/ui_node.cpp
      src/renderer/ui/ui_window.cpp
)

target_include_directories(other_renderer 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/renderer>
)

target_link_directories(other_renderer PUBLIC 
  $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
  $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)
target_link_libraries(other_renderer PRIVATE imgui SDL3 glad ${ASSIMP_NAME} other_core)

other_install_module(other_renderer "renderer")