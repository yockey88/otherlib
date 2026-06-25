add_library(imgui STATIC)
target_sources(imgui
    PRIVATE
        imgui/misc/freetype/imgui_freetype.cpp       
        imgui/misc/cpp/imgui_stdlib.cpp       
        imguizmo/ImGuizmo.cpp
        imguizmo/ImVectorEditor.cpp
        imguizmo/ImSequencer.cpp
        imguizmo/ImGradient.cpp
        imguizmo/ImCurveEdit.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)
set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/freetype ${CMAKE_CURRENT_SOURCE_DIR}/../sdl)
set(IMGUI_INCLUDE_DIRS ${IMGUI_INCLUDE_DIRS} PARENT_SCOPE)

### TODO fix this to use interface generator expressions $<...>
target_include_directories(imgui PRIVATE ${IMGUI_INCLUDE_DIRS})

other_install_dependency(imgui "${CMAKE_CURRENT_SOURCE_DIR}/imgui" HEADER_PATTERNS "*.h" "*.hpp")