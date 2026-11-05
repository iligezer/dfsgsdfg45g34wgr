cmake_minimum_required(VERSION 3.15)
project(UnityMonoResearch)

set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_MBCS)

# Ищем ImGui и MinHook
find_package(imgui CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)

add_library(UnityMonoResearch SHARED
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
)

target_link_libraries(UnityMonoResearch PRIVATE 
    imgui::imgui 
    minhook::minhook
    d3d11
    dxgi
)
