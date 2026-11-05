cmake_minimum_required(VERSION 3.20)
project(UnityMonoResearch CXX)

# ============================================================
# 32-БИТНАЯ СБОРКА (ВАЖНО ДЛЯ MONO-ИГР)
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Принудительно 32-бит (x86)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "This project must be built for 32-bit (x86). Use -A Win32 in Visual Studio.")
endif()

# ============================================================
# ЗАВИСИМОСТИ (ImGui, MinHook)
# ============================================================
# Ожидается структура:
#   vendor/imgui/            (imgui.cpp, imgui.h, ...)
#   vendor/imgui/backends/   (imgui_impl_win32.cpp, imgui_impl_dx11.cpp, ...)
#   vendor/minhook/          (include/MinHook.h, src/*.c)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")
set(MINHOOK_DIR "${CMAKE_SOURCE_DIR}/vendor/minhook")

# ---- ImGui ----
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

# ---- MinHook (C-файлы) ----
file(GLOB MINHOOK_SOURCES
    ${MINHOOK_DIR}/src/*.c
)

# ============================================================
# ИСХОДНИКИ ПРОЕКТА
# ============================================================
set(PROJECT_SOURCES
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
    src/Overlay.cpp
    src/Hook.cpp
)

# ============================================================
# СБОРКА DLL
# ============================================================
add_library(UnityMonoResearch SHARED
    ${PROJECT_SOURCES}
    ${IMGUI_SOURCES}
    ${MINHOOK_SOURCES}
)

# ============================================================
# INCLUDE DIRECTORIES
# ============================================================
target_include_directories(UnityMonoResearch PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${MINHOOK_DIR}/include
)

# ============================================================
# БИБЛИОТЕКИ WINDOWS (DX11, DXGI, PSAPI)
# ============================================================
target_link_libraries(UnityMonoResearch PRIVATE
    d3d11
    dxgi
    dxguid
    Psapi
    user32
    gdi32
)

# ============================================================
# НАСТРОЙКИ КОМПИЛЯТОРА
# ============================================================
target_compile_definitions(UnityMonoResearch PRIVATE
    _CRT_SECURE_NO_WARNINGS
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(UnityMonoResearch PRIVATE
        /MP          # Многопоточная компиляция
        /W3          # Уровень предупреждений
    )
endif()

# ============================================================
# ВЫХОДНОЙ ФАЙЛ (DLL)
# ============================================================
set_target_properties(UnityMonoResearch PROPERTIES
    PREFIX ""
    OUTPUT_NAME "UnityMonoResearch"
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "==============================================")
message(STATUS "Project: UnityMonoResearch (32-bit DLL)")
message(STATUS "ImGui:   ${IMGUI_DIR}")
message(STATUS "MinHook: ${MINHOOK_DIR}")
message(STATUS "Output:  ${CMAKE_BINARY_DIR}/bin")
message(STATUS "==============================================")
