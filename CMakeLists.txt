cmake_minimum_required(VERSION 3.15)
project(UnityMonoResearch)

set(CMAKE_CXX_STANDARD 17)

# Ищем только ImGui через vcpkg
find_package(imgui CONFIG REQUIRED)

add_library(UnityMonoResearch SHARED
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
)

# Линкуем ImGui и стандартную библиотеку DirectX 11 (она уже есть в Windows SDK)
target_link_libraries(UnityMonoResearch PRIVATE 
    imgui::imgui 
    d3d11
    dxgi
)
