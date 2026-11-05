cmake_minimum_required(VERSION 3.15)
project(UnityMonoResearch)

set(CMAKE_CXX_STANDARD 17)

# Подключаем ImGui и DirectX 11 через vcpkg
find_package(imgui CONFIG REQUIRED)
find_package(directx11 REQUIRED)

add_library(UnityMonoResearch SHARED
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
)

target_link_libraries(UnityMonoResearch PRIVATE imgui::imgui d3d11)
