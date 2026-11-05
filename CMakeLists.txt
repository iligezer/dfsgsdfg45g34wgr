cmake_minimum_required(VERSION 3.15)
project(UnityMonoResearch)

set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_MBCS)

find_package(imgui CONFIG REQUIRED)

add_library(UnityMonoResearch SHARED
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
)

target_link_libraries(UnityMonoResearch PRIVATE 
    imgui::imgui 
    d3d11
    dxgi
)
