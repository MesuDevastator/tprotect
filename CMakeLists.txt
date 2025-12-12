# CMakeLists.txt: The CMake Build Script

cmake_minimum_required(VERSION 3.29)
project(TProtect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)

file(GLOB_RECURSE TPROTECT_SRCS src/*.cpp)
add_executable(tprotect ${IMGUI_SRCS} ${TPROTECT_SRCS})
target_include_directories(tprotect PRIVATE include)
add_dependencies(tprotect imgui ImGuiFileDialog)
if(APPLE)
    target_link_libraries(tprotect PRIVATE "-framework OpenGL")
elseif(WIN32)
    target_link_libraries(tprotect PRIVATE opengl32)
elseif(UNIX)
    target_link_libraries(tprotect PRIVATE GL)
endif()
target_link_libraries(tprotect PRIVATE imgui ImGuiFileDialog)
