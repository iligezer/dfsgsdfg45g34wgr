cmake_minimum_required(VERSION 3.15)
project(UnityMonoResearch)

set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_MBCS)

find_package(imgui CONFIG REQUIRED)

# Указываем путь к MinHook напрямую (vcpkg складывает его сюда)
set(MINHOOK_DIR "$ENV{VCPKG_ROOT}/installed/x86-windows-static")

add_library(UnityMonoResearch SHARED
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
)

target_include_directories(UnityMonoResearch PRIVATE 
    "${MINHOOK_DIR}/include"
)

target_link_libraries(UnityMonoResearch PRIVATE 
    imgui::imgui 
    "${MINHOOK_DIR}/lib/minhook.lib"  # <-- Прямая ссылка на статическую библиотеку
    d3d11
    dxgi
)
