add_subdirectory(glfw)
add_subdirectory(freetype)
add_subdirectory(plutosvg)
add_subdirectory(ImGuiFileDialog)

file(GLOB IMGUI_SRCS imgui/*.cpp)
set(IMGUI_SRCS
    ${IMGUI_SRCS}
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
    imgui/misc/freetype/imgui_freetype.cpp
)
add_library(imgui ${IMGUI_SRCS})
target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
    imgui/misc/cpp
    imgui/misc/freetype
)
target_compile_definitions(imgui PUBLIC
    IMGUI_DISABLE_DEFAULT_FONT
    IMGUI_ENABLE_FREETYPE
    IMGUI_ENABLE_FREETYPE_PLUTOSVG
)
add_dependencies(imgui freetype plutosvg glfw)
target_link_libraries(imgui PUBLIC freetype plutosvg glfw)
target_link_libraries(ImGuiFileDialog PUBLIC imgui)
