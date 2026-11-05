cmake_minimum_required(VERSION 3.20)
project(UnityMonoResearch CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "This project must be built for 32-bit (x86). Use -A Win32")
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/MonoAPI.cpp
    src/GameReader.cpp
)

add_library(UnityMonoResearch SHARED ${PROJECT_SOURCES})

target_include_directories(UnityMonoResearch PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(UnityMonoResearch PRIVATE
    d3d11
    dxgi
    dxguid
    Psapi
)

target_compile_definitions(UnityMonoResearch PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set_target_properties(UnityMonoResearch PROPERTIES
    PREFIX ""
    OUTPUT_NAME "UnityMonoResearch"
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
