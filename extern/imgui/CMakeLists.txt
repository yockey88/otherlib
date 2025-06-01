add_library(imgui STATIC)
target_sources(imgui
    PRIVATE
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/misc/cpp/imgui_stdlib.cpp       
        imgui/misc/freetype/imgui_freetype.cpp       
)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/../sdl)